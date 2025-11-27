/* tests/fork_scheduler_integration_test.c - integration test for fork + scheduler */

#include <stdio.h>
#include <string.h>

/* Host-side test uses HOST_TEST mode for kmalloc */
#define HOST_TEST

#include <stdint.h>
#include "../kernel/mm/virtual_memory.c"
#include "../kernel/process_manager.c"
#include "../kernel/mm/pagetable.c"
#include "../kernel/scheduler/preemptive.c"

void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t v) { printf("%llx", (unsigned long long)v); }

/* Host-side stubs for hardware helpers */
void enable_interrupts(void) { /* no-op for host tests */ }
void pic_send_eoi(int irq) { (void)irq; /* no-op */ }

int main(void) {
    /* Create a parent process with an allocated stack that contains a saved-regs frame */
    process_t *parent = (process_t *)kmalloc(sizeof(process_t));
    if (!parent) { printf("ERROR: kmalloc parent failed\n"); return 1; }
    memset(parent, 0, sizeof(process_t));
    parent->entry_point = 0x1000;

    const int REG_COUNT = 15;
    const uint64_t STKSZ = 0x1000;
    parent->stack_base = (uint64_t)kmalloc(STKSZ);
    parent->stack_size = STKSZ;
    parent->stack_top = parent->stack_base + STKSZ - (REG_COUNT + 3) * sizeof(uint64_t);

    /* set parent saved RAX to a non-zero value so we can detect child's 0 */
    uint64_t *p_saved = (uint64_t *)parent->stack_top;
    p_saved[REG_COUNT - 1] = 0xCAFEBABEULL;

    pm_register_process(parent);
    pm_set_current(parent);

    /* Clone using pm_clone_process (child->saved RAX should be 0) */
    process_t *child = pm_clone_process(parent);
    if (!child) { printf("ERROR: pm_clone_process failed\n"); return 1; }

    /* Add both to scheduler */
    sched_add_existing_process(parent);
    sched_add_existing_process(child);

    /* Simulate timer tick: first call returns first task's stack_top (parent or child) */
    uint64_t fake_saved_area[64];

    uint64_t ret1 = scheduler_tick(fake_saved_area);
    uint64_t ret2 = scheduler_tick(fake_saved_area);

    if (ret1 == 0 || ret2 == 0) { printf("FAIL: scheduler_tick returned 0\n"); return 1; }

    if (ret1 == ret2) { printf("FAIL: expected different stack_top pointers got same\n"); return 1; }

    /* Identify which returned pointer belongs to child and verify child's saved RAX == 0 */
    uint64_t *cand1 = (uint64_t *)ret1;
    uint64_t *cand2 = (uint64_t *)ret2;

    int child_ok = 0;
    if (cand1[REG_COUNT - 1] == 0) child_ok = 1;
    if (cand2[REG_COUNT - 1] == 0) child_ok = 1;

    if (!child_ok) {
        printf("FAIL: child saved RAX not zero in either returned context (0x%llx,0x%llx)\n", (unsigned long long)cand1[REG_COUNT - 1], (unsigned long long)cand2[REG_COUNT - 1]);
        return 1;
    }

    printf("PASS: fork + scheduler integration shows child saved RAX == 0\n");
    return 0;
}
