/* kernel/mm/virtual_memory.c - stubs for virtual memory interface */
#include <stdint.h>
#include "virtual_memory.h"
#ifdef HOST_TEST
#include <stdlib.h>
#endif

/* Simple bump allocator for Phase 1 */
static uint64_t heap_ptr = 0x10000000;

void *virtual_memory_alloc(uint64_t vaddr, uint64_t size, int prot) {
    (void)prot; /* ignore protection for now */
    
    if (vaddr == 0) {
        /* Auto-allocate at heap */
        void *ptr = (void *)heap_ptr;
        heap_ptr += (size + 0xFFF) & ~0xFFF;  /* align to 4K */
        return ptr;
    } else {
        /* Fixed-address allocation */
        return (void *)vaddr;
    }
}

void virtual_memory_free(uint64_t vaddr, uint64_t size) {
    (void)vaddr;
    (void)size;
    /* TODO: implement heap defragmentation */
}

int virtual_memory_map(uint64_t vaddr, uint64_t paddr, int prot) {
    (void)vaddr;
    (void)paddr;
    (void)prot;
    /* TODO: update page tables */
    return 0;
}

/* Legacy kmalloc stub */
void *kmalloc(unsigned int size) {
#ifdef HOST_TEST
    /* When running host-side unit tests, fall back to malloc so pointers
       are valid in the host process address space. */
    return malloc(size);
#else
    static uint64_t heap = 0x01000000; /* 16MB start */
    void *ptr = (void *)heap;
    heap += size;
    return ptr;
#endif
}

