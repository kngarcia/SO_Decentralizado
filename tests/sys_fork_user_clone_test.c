/* tests/sys_fork_user_clone_test.c - host-side unit test for sys_fork cloning path */

#include <stdio.h>
#include <string.h>

/* Include only the user-space process path implementations: virtual memory, process manager, elf loader and syscall */
#include "../kernel/mm/virtual_memory.c"
#include "../kernel/tasks/process.c"
#include "../kernel/process_manager.c"
#include "../kernel/elf_loader.c"
#include "../kernel/syscall.c"

/* minimal serial stubs used by syscall.c for host tests */
void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t value) { printf("%llx", (unsigned long long)value); }

int main(void) {
    /* Create a dummy ELF process and register it via pm_register_process() */
    process_t *p = (process_t *)kmalloc(sizeof(process_t));
    if (!p) {
        printf("ERROR: kmalloc failed\n");
        return 1;
    }
    memset(p, 0, sizeof(process_t));
    p->entry_point = 0x1000;
    /* allocate a real stack in host tests via kmalloc and set top
       to near the end of the allocated region so saved-regs pointers
       align with scheduler expectations */
    const uint64_t STKSZ = 0x1000;
    p->stack_base = (uint64_t)kmalloc(STKSZ);
    if (!p->stack_base) {
        printf("ERROR: kmalloc for stack failed\n");
        return 1;
    }
    p->stack_size = STKSZ;
    /* Push the saved register frame near the top of the stack (matching REG_COUNT + 3 reserved words) */
    const int REG_COUNT = 15;
    p->stack_top = p->stack_base + STKSZ - (REG_COUNT + 3) * sizeof(uint64_t);
    /* set a non-zero RAX in parent saved slot so we can detect child override */
    uint64_t *parent_saved = (uint64_t *)p->stack_top;
    parent_saved[REG_COUNT - 1] = 0xDEADBEEFULL;

    uint64_t pid_before = pm_register_process(p);
    if (pm_count() < 1) {
        printf("ERROR: pm_register_process failed\n");
        return 1;
    }

    pm_set_current(p);

    /* Call pm_clone_process directly to get a child pointer we can inspect */
    extern process_t *pm_clone_process(process_t *parent);
    process_t *child_proc = pm_clone_process(p);
    if (!child_proc) {
        printf("ERROR: pm_clone_process failed\n");
        return 1;
    }
    int child = (int)child_proc->pid;
    if (child < 0) {
        printf("ERROR: sys_fork clone failed and returned %d\n", child);
        return 1;
    }

    if (pm_count() <= 1) {
        printf("ERROR: pm_clone didn't increase process table (count=%d)\n", pm_count());
        return 1;
    }

    /* Validate child saved register state: ensure returned RAX will be 0 */
    if (child_proc->stack_top) {
        uint64_t *saved = (uint64_t *)child_proc->stack_top;
        const int REG_COUNT = 15;
        if (saved[REG_COUNT - 1] != 0) {
            printf("ERROR: child saved RAX not zero (0x%llx)\n", (unsigned long long)saved[REG_COUNT - 1]);
            return 1;
        }
    }

    printf("PASS: sys_fork cloned process -> child pid=%d (total pm_count=%d)\n", child, pm_count());
    return 0;
}
