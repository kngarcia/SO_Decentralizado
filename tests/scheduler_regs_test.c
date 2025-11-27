/* tests/scheduler_regs_test.c - verify scheduler preserves saved registers across switch */

#include <stdio.h>
#include <string.h>
#define HOST_TEST

#include "../kernel/process_manager.c"
#include "../kernel/mm/pagetable.c"
#include "../kernel/scheduler/preemptive.c"
#include "../kernel/mm/virtual_memory.c"

void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t v) { printf("%llx", (unsigned long long)v); }

void enable_interrupts(void) {}
void pic_send_eoi(int irq) { (void)irq; }

int main(void) {
    const int REG_COUNT = 15;
    const uint64_t STKSZ = 0x1000;

    /* Allocate two processes and set saved register frames with unique patterns */
    process_t *p1 = (process_t *)kmalloc(sizeof(process_t));
    process_t *p2 = (process_t *)kmalloc(sizeof(process_t));
    memset(p1, 0, sizeof(process_t));
    memset(p2, 0, sizeof(process_t));

    p1->stack_base = (uint64_t)kmalloc(STKSZ);
    p1->stack_size = STKSZ;
    p1->stack_top = p1->stack_base + STKSZ - (REG_COUNT + 3) * sizeof(uint64_t);

    p2->stack_base = (uint64_t)kmalloc(STKSZ);
    p2->stack_size = STKSZ;
    p2->stack_top = p2->stack_base + STKSZ - (REG_COUNT + 3) * sizeof(uint64_t);

    uint64_t *s1 = (uint64_t *)p1->stack_top;
    uint64_t *s2 = (uint64_t *)p2->stack_top;
    for (int i = 0; i < REG_COUNT; ++i) {
        s1[i] = 0x100 + i;
        s2[i] = 0x200 + i;
    }

    pm_register_process(p1);
    pm_register_process(p2);
    sched_add_existing_process(p1);
    sched_add_existing_process(p2);

    uint64_t saved_area[64];
    /* First tick, scheduler should pick first task's stack_top */
    uint64_t r1 = scheduler_tick(saved_area);
    uint64_t r2 = scheduler_tick(saved_area);

    if (r1 == 0 || r2 == 0) { printf("ERROR: scheduler returned null\n"); return 1; }

    uint64_t *arr1 = (uint64_t *)r1;
    uint64_t *arr2 = (uint64_t *)r2;

    /* Confirm saved register patterns preserved */
    int ok = 1;
    for (int i = 0; i < REG_COUNT; ++i) {
        if (!(arr1[i] == (uint64_t)(0x100 + i) || arr1[i] == (uint64_t)(0x200 + i))) ok = 0;
        if (!(arr2[i] == (uint64_t)(0x100 + i) || arr2[i] == (uint64_t)(0x200 + i))) ok = 0;
    }

    if (!ok) { printf("FAIL: saved registers not preserved\n"); return 1; }

    printf("PASS: scheduler preserves saved registers across switches\n");
    return 0;
}
