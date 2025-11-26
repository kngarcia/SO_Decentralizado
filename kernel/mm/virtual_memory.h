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

#endif /* VIRTUAL_MEMORY_H */
