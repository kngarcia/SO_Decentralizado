/* tests/cow_test.c - copy-on-write behavior tests (host) */

#include <stdio.h>
#include <string.h>
#define HOST_TEST

#include "../kernel/mm/virtual_memory.c"
#include "../kernel/process_manager.c"
#include "../kernel/mm/pagetable.c"
#include "../kernel/scheduler/preemptive.c"

void serial_puts(const char *s) { if (s) printf("%s", s); }
void serial_putc(char c) { putchar(c); }
void serial_put_hex(uint64_t v) { printf("%llx", (unsigned long long)v); }

/* Host stubs for scheduler/hardware interactions */
void enable_interrupts(void) { }
void pic_send_eoi(int irq) { (void)irq; }

int main(void) {
    /* Allocate a region (parent) */
    uint64_t addr = (uint64_t)virtual_memory_alloc(0, 0x1000, PROT_READ | PROT_WRITE);
    if (!addr) { printf("ERROR: virt alloc failed\n"); return 1; }

    int rc1 = virtual_memory_refcount(addr);
    if (rc1 != 1) { printf("ERROR: expected refcount=1 got %d\n", rc1); return 1; }

    /* Create a process that uses this heap region */
    process_t *p = (process_t *)kmalloc(sizeof(process_t));
    memset(p, 0, sizeof(process_t));
    p->heap_start = addr;
    p->heap_end = addr + 0x1000;

    pm_register_process(p);
    pm_set_current(p);

    /* Clone the process -> heap should be shared (refcount increments) */
    process_t *child = pm_clone_process(p);
    if (!child) { printf("ERROR: clone failed\n"); return 1; }

    int rc2 = virtual_memory_refcount(addr);
    if (rc2 < 2) { printf("ERROR: expected refcount>=2 got %d\n", rc2); return 1; }

    /* Make the child's heap writable (simulate write causing COW) */
    void *ptr = virtual_memory_make_writable(child->heap_start, 0x1000);
    if (!ptr) { printf("ERROR: virtual_memory_make_writable failed\n"); return 1; }

    int rc3 = virtual_memory_refcount(addr);
    if (rc3 != 1) { printf("ERROR: expected original refcount to decrease to 1, got %d\n", rc3); return 1; }

    printf("PASS: COW semantics simulated on host (refcounts %d->%d->%d)\n", rc1, rc2, rc3);
    return 0;
}
