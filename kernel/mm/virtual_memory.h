/* kernel/mm/virtual_memory.h
 * Virtual memory management stubs for Phase 1
 */

#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H

#include <stdint.h>
#include <stddef.h>

/* Memory protection flags */
#define PROT_READ   0x01
#define PROT_WRITE  0x02
#define PROT_EXEC   0x04

/* Allocate virtual memory region */
void *virtual_memory_alloc(uint64_t vaddr, uint64_t size, int prot);

/* Free virtual memory region */
void virtual_memory_free(uint64_t vaddr, uint64_t size);

/* Map physical page to virtual address */
int virtual_memory_map(uint64_t vaddr, uint64_t paddr, int prot);

/* Copy-on-write helpers (Phase1 simplified) */
void virtual_memory_share(uint64_t vaddr, uint64_t size);
/* Ensure a writable mapping for the region containing vaddr. If region is
	shared (refcount>1), allocate private copy and return new host pointer.
	Returns host pointer to writable buffer on success, NULL on failure. */
void *virtual_memory_make_writable(uint64_t vaddr, uint64_t size);

/* For tests: get reference count for region starting at vaddr */
int virtual_memory_refcount(uint64_t vaddr);

/* Page-fault handler helper used by isr_0x0e wrapper
 * Saved_regs_ptr: pointer to saved registers block
 * fault_addr: CR2
 * Returns the next RSP for the ISR to resume (usually same saved_regs_ptr)
 */
uint64_t page_fault_handler(uint64_t *saved_regs_ptr, uint64_t fault_addr);

#endif /* VIRTUAL_MEMORY_H */
