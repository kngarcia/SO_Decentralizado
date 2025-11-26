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
    p->stack_top = 0x8000;

    uint64_t pid_before = pm_register_process(p);
    if (pm_count() < 1) {
        printf("ERROR: pm_register_process failed\n");
        return 1;
    }

    pm_set_current(p);

    int child = sys_fork();
    if (child < 0) {
        printf("ERROR: sys_fork clone failed and returned %d\n", child);
        return 1;
    }

    if (pm_count() <= 1) {
        printf("ERROR: pm_clone didn't increase process table (count=%d)\n", pm_count());
        return 1;
    }

    printf("PASS: sys_fork cloned process -> child pid=%d (total pm_count=%d)\n", child, pm_count());
    return 0;
}
