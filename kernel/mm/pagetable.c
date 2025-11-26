/* kernel/mm/pagetable.c - minimal page table helpers for Phase1
 * clone current PML4, set CR3, and return kernel PML4 pointer.
 */
#include "pagetable.h"
#include "virtual_memory.h"
#include <string.h>

#ifdef HOST_TEST
#include <stdlib.h>
/* In host tests, emulate a kernel pml4 array */
static uint64_t host_pml4[512];

void *pt_get_kernel_pml4(void) { return host_pml4; }

void *pt_clone_current(void) {
    void *p = malloc(4096);
    if (!p) return NULL;
    memcpy(p, host_pml4, 4096);
    return p;
}

void pt_set_cr3(void *p) { (void)p; /* no-op in host tests */ }

#else
/* In kernel builds, use existing pml4 symbol exported by start.S (identity mapping) */
extern uint64_t pml4[];

void *pt_get_kernel_pml4(void) { return (void *)pml4; }

/* Clone current kernel pml4 (simple memory copy) */
void *pt_clone_current(void) {
    void *new = kmalloc(4096);
    if (!new) return NULL;
    memcpy(new, (void *)pml4, 4096);
    return new;
}

void pt_set_cr3(void *p) {
    /* p is page-aligned physical address (identity mapped). Use MOV to CR3 */
    asm volatile ("mov %0, %%cr3" :: "r"(p));
}

#endif
