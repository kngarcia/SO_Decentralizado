/* kernel/mm/pagetable.h - simple helpers for cloning/setting CR3 */
#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <stdint.h>

void *pt_clone_current(void);
void pt_set_cr3(void *p);
void *pt_get_kernel_pml4(void);

/* Mark page table entries as user-accessible recursively for vaddr */
void pt_mark_user_recursive(void *pml4_base, uint64_t vaddr);

/* Helpers for page-table walk (return pointer to PTE for vaddr in the given PML4 base, or NULL) */
uint64_t *pt_find_pte_for_vaddr(void *pml4_base, uint64_t vaddr);

/* Clone PML4 for fork with Copy-On-Write semantics: returns new PML4 pointer */
void *pt_clone_for_cow(void *parent_pml4);

/* Map a virtual range into the given PML4 with newly allocated frames.
	Size will be rounded up to 4KB. Flags should include present (bit0),
	writable (bit1) and user (bit2) as needed. Returns 0 on success. */
int pt_map_range(void *pml4_base, uint64_t vaddr, uint64_t size, uint64_t flags);

#endif
