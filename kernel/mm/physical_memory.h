/* kernel/mm/physical_memory.h - public prototypes for physical memory helpers */
#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#include <stdint.h>

/* Initialize physical memory allocator */
void physical_memory_init(void);

/* Allocate/free frames */
uint32_t alloc_frame(void);
void free_frame(uint32_t addr);

/* Frame reference counting for COW */
void frame_incref(uint32_t addr);
void frame_decref(uint32_t addr);
int frame_refcount_get(uint32_t addr);

/* Utility */
uint32_t first_free_frame(void);

#endif
