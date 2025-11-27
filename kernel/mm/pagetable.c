/* kernel/mm/pagetable.c - minimal page table helpers for Phase1
 * clone current PML4, set CR3, and return kernel PML4 pointer.
 */
#include "pagetable.h"
#include "virtual_memory.h"
#include <string.h>
#include "physical_memory.h"

#ifdef HOST_TEST
#include <stdlib.h>
/* In host tests, emulate a kernel pml4 array */
static uint64_t host_pml4[512];

void *pt_get_kernel_pml4(void) { return host_pml4; }

extern void *kmalloc(unsigned int size);

void *pt_clone_current(void) {
    void *p = malloc(4096);
    if (!p) return NULL;
    memcpy(p, host_pml4, 4096);
    return p;
}

/* Walk page tables and find pointer to final PTE for vaddr (or NULL if not present)
 * pml4_base is a pointer to the root PML4 table (identity mapped).
 */
uint64_t *pt_find_pte_for_vaddr(void *pml4_base, uint64_t vaddr) {
    uint64_t *pml4 = (uint64_t *)pml4_base;
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FFULL;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FFULL;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FFULL;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FFULL;

    uint64_t pml4e = pml4[pml4_idx];
    if (!(pml4e & 1)) return NULL; /* not present */
    uint64_t *pdpt = (uint64_t *)((uint64_t)(pml4e & ~0xFFFULL));

    uint64_t pdpte = pdpt[pdpt_idx];
    if (!(pdpte & 1)) return NULL;
    /* Check for 1GB page (PS bit in PDPT) */
    if (pdpte & (1ULL<<7)) {
        /* No PT entry exists for 1GB pages; return pointer to PDPT entry (address)
           so caller may detect large page */
        return &pdpt[pdpt_idx];
    }

    uint64_t *pd = (uint64_t *)((uint64_t)(pdpte & ~0xFFFULL));
    uint64_t pde = pd[pd_idx];
    if (!(pde & 1)) return NULL;
    /* Check for 2MB page (PS bit in PD) */
    if (pde & (1ULL<<7)) {
        return &pd[pd_idx];
    }

    uint64_t *pt = (uint64_t *)((uint64_t)(pde & ~0xFFFULL));
    return &pt[pt_idx];
}

/* Clone the current PML4 and apply copy-on-write semantics for all mapped PT entries
 * - Allocates a page for new PML4
 * - Copies the PML4 content
 * - Walks all present PML4->PDPT->PD->PT entries and for each present 4KB PTE:
 *   - increments frame refcount
 *   - clears the writable bit in both parent and child entries so writes will cause PF
 */
void *pt_clone_for_cow(void *parent_pml4) {
    if (!parent_pml4) return NULL;
    void *new_pml4 = kmalloc(4096);
    if (!new_pml4) return NULL;
    memcpy(new_pml4, parent_pml4, 4096);

    /* Walk PML4 entries */
    uint64_t *orig = (uint64_t *)parent_pml4;
    uint64_t *copy = (uint64_t *)new_pml4;
    for (int i = 0; i < 512; ++i) {
        uint64_t pml4e = orig[i];
        if (!(pml4e & 1)) continue; /* not present */
        uint64_t *pdpt = (uint64_t *)((uint64_t)(pml4e & ~0xFFFULL));
        uint64_t *pdpt_copy = (uint64_t *)((uint64_t)(copy[i] & ~0xFFFULL));
        if (!pdpt_copy) continue;
        for (int j = 0; j < 512; ++j) {
            uint64_t pdpte = pdpt[j];
            if (!(pdpte & 1)) continue;
            /* big 1GB pages? leave unchanged (hard to COW now) */
            if (pdpte & (1ULL<<7)) continue;
            uint64_t *pd = (uint64_t *)((uint64_t)(pdpte & ~0xFFFULL));
            uint64_t *pd_copy = (uint64_t *)((uint64_t)(pdpt_copy[j] & ~0xFFFULL));
            if (!pd_copy) continue;
            for (int k = 0; k < 512; ++k) {
                uint64_t pde = pd[k];
                if (!(pde & 1)) continue;
                if (pde & (1ULL<<7)) continue; /* 2MB page, leave for now */
                uint64_t *pt = (uint64_t *)((uint64_t)(pde & ~0xFFFULL));
                uint64_t *pt_copy = (uint64_t *)((uint64_t)(pd_copy[k] & ~0xFFFULL));
                if (!pt_copy) continue;
                for (int m = 0; m < 512; ++m) {
                    uint64_t pte = pt[m];
                    if (!(pte & 1)) continue;
                    /* physical frame address */
                    uint32_t frame = (uint32_t)(pte & ~0xFFFULL);
                    if (frame == 0) continue;
                    /* increment refcount */
                    frame_incref(frame);
                    /* Clear writable bit on both parent and child PTEs */
                    pt[m] = pte & ~0x2ULL; /* clear writable */
                    pt_copy[m] = pt[m];
                }
            }
        }
    }

    return new_pml4;
}

void pt_set_cr3(void *p) { (void)p; /* no-op in host tests */ }

#else
/* In kernel builds, use existing pml4 symbol exported by start.S (identity mapping) */
extern uint64_t pml4[];

void *pt_get_kernel_pml4(void) { return (void *)pml4; }

/* Page-table helpers available in kernel build */
/* kmalloc is defined in virtual_memory.c; declare it here so kernel builds
    don't get implicit-declaration warnings when using kmalloc. */
extern void *kmalloc(unsigned int size);
uint64_t *pt_find_pte_for_vaddr(void *pml4_base, uint64_t vaddr) {
    uint64_t *pml4 = (uint64_t *)pml4_base;
    uint64_t pml4_idx = (vaddr >> 39) & 0x1FFULL;
    uint64_t pdpt_idx = (vaddr >> 30) & 0x1FFULL;
    uint64_t pd_idx = (vaddr >> 21) & 0x1FFULL;
    uint64_t pt_idx = (vaddr >> 12) & 0x1FFULL;

    uint64_t pml4e = pml4[pml4_idx];
    if (!(pml4e & 1)) return NULL; /* not present */
    uint64_t *pdpt = (uint64_t *)((uint64_t)(pml4e & ~0xFFFULL));

    uint64_t pdpte = pdpt[pdpt_idx];
    if (!(pdpte & 1)) return NULL;
    if (pdpte & (1ULL<<7)) return &pdpt[pdpt_idx];

    uint64_t *pd = (uint64_t *)((uint64_t)(pdpte & ~0xFFFULL));
    uint64_t pde = pd[pd_idx];
    if (!(pde & 1)) return NULL;
    if (pde & (1ULL<<7)) return &pd[pd_idx];

    uint64_t *pt = (uint64_t *)((uint64_t)(pde & ~0xFFFULL));
    return &pt[pt_idx];
}

void *pt_clone_for_cow(void *parent_pml4) {
    if (!parent_pml4) return NULL;
    void *new_pml4 = kmalloc(4096);
    if (!new_pml4) return NULL;
    memcpy(new_pml4, parent_pml4, 4096);

    uint64_t *orig = (uint64_t *)parent_pml4;
    uint64_t *copy = (uint64_t *)new_pml4;
    for (int i = 0; i < 512; ++i) {
        uint64_t pml4e = orig[i];
        if (!(pml4e & 1)) continue;
        uint64_t *pdpt = (uint64_t *)((uint64_t)(pml4e & ~0xFFFULL));
        uint64_t *pdpt_copy = (uint64_t *)((uint64_t)(copy[i] & ~0xFFFULL));
        if (!pdpt_copy) continue;
        for (int j = 0; j < 512; ++j) {
            uint64_t pdpte = pdpt[j];
            if (!(pdpte & 1)) continue;
            if (pdpte & (1ULL<<7)) continue;
            uint64_t *pd = (uint64_t *)((uint64_t)(pdpte & ~0xFFFULL));
            uint64_t *pd_copy = (uint64_t *)((uint64_t)(pdpt_copy[j] & ~0xFFFULL));
            if (!pd_copy) continue;
            for (int k = 0; k < 512; ++k) {
                uint64_t pde = pd[k];
                if (!(pde & 1)) continue;
                if (pde & (1ULL<<7)) continue;
                uint64_t *pt = (uint64_t *)((uint64_t)(pde & ~0xFFFULL));
                uint64_t *pt_copy = (uint64_t *)((uint64_t)(pd_copy[k] & ~0xFFFULL));
                if (!pt_copy) continue;
                for (int m = 0; m < 512; ++m) {
                    uint64_t pte = pt[m];
                    if (!(pte & 1)) continue;
                    uint32_t frame = (uint32_t)(pte & ~0xFFFULL);
                    if (frame == 0) continue;
                    frame_incref(frame);
                    pt[m] = pte & ~0x2ULL;
                    pt_copy[m] = pt[m];
                }
            }
        }
    }

    return new_pml4;
}

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
