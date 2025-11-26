/* tests/preemptive_scheduler_test.c - host-side test for scheduler_tick */

#include <stdio.h>
#include <string.h>

/* Host-side test uses HOST_TEST mode for kmalloc */
#define HOST_TEST

#include <stdint.h>
#include "../kernel/mm/virtual_memory.c"
#include "../kernel/process_manager.c"
#include "../kernel/scheduler/preemptive.c"

void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t v) { printf("%llx", (unsigned long long)v); }

/* Host-side stubs for hardware helpers */
void enable_interrupts(void) { /* no-op for host tests */ }
void pic_send_eoi(int irq) { (void)irq; /* no-op */ }

int main(void) {
    /* Create two kernel tasks using the scheduler task_create helper */
    void dummy1(void) { for (;;) asm volatile("hlt"); }
    void dummy2(void) { for (;;) asm volatile("hlt"); }

    /* Create two process_t structs and register them with process manager */
    process_t *p1 = (process_t *)kmalloc(sizeof(process_t));
    process_t *p2 = (process_t *)kmalloc(sizeof(process_t));
    memset(p1, 0, sizeof(process_t));
    memset(p2, 0, sizeof(process_t));
    p1->entry_point = 0x1000; p2->entry_point = 0x2000;

    p1->stack_top = 0xAAA000;
    p2->stack_top = 0xBBB000;
    pm_register_process(p1);
    pm_register_process(p2);

    /* Add them into the scheduler task list (test helper) */
    sched_add_existing_process(p1);
    sched_add_existing_process(p2);

    if (pm_count() < 2) {
        printf("ERROR: expected at least 2 registered processes, got %d\n", pm_count());
        return 1;
    }

    /* Simulate tick: saved_regs_ptr can be any pointer; scheduler_tick should
       save current's stack_top and return next's stack_top */
    uint64_t fake_saved_area[64];
    uint64_t ret1 = scheduler_tick(fake_saved_area);
    if (ret1 == 0) {
        printf("FAIL: scheduler_tick returned 0 first\n");
        return 1;
    }

    uint64_t ret2 = scheduler_tick(fake_saved_area);
    if (ret2 == 0) {
        printf("FAIL: scheduler_tick returned 0 second\n");
        return 1;
    }

    if (ret1 == ret2) {
        printf("FAIL: expected ret1 != ret2 but both equal 0x%llx\n", (unsigned long long)ret1);
        return 1;
    }

    printf("PASS: scheduler_tick rotated tasks correctly (ret1=0x%llx ret2=0x%llx)\n", (unsigned long long)ret1, (unsigned long long)ret2);
    return 0;
}
