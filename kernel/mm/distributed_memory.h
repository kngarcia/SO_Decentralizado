/* kernel/mm/distributed_memory.h */
#ifndef DISTRIBUTED_MEMORY_H
#define DISTRIBUTED_MEMORY_H

#include <stdint.h>

/* Initialize DSM subsystem */
int dsm_init(uint32_t node_id);

/* Allocate distributed shared memory */
void *dsm_alloc(uint64_t size, int replicas);

/* Free distributed shared memory */
void dsm_free(void *vaddr);

/* Synchronize page with owner node */
int dsm_sync_page(uint64_t vaddr, uint32_t owner_node);

/* Handle remote page fault */
int dsm_handle_remote_fault(uint64_t vaddr, uint32_t faulting_node);

/* Invalidate page on all nodes */
int dsm_invalidate_page(uint64_t vaddr);

/* Print DSM statistics */
void dsm_print_stats(void);

#endif /* DISTRIBUTED_MEMORY_H */
