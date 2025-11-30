/* kernel/mm/mmio.h
 * Memory-Mapped I/O (MMIO) management
 * Maps physical device memory into kernel virtual address space
 */

#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>
#include <stddef.h>

/* MMIO virtual address space range (high canonical addresses)
 * x86-64: 0xFFFF800000000000 - 0xFFFF880000000000 (512 GB)
 */
#define MMIO_VIRT_BASE  0xFFFF800000000000ULL
#define MMIO_VIRT_END   0xFFFF880000000000ULL
#define MMIO_MAX_SIZE   (128ULL * 1024 * 1024)  // 128 MB max per device

/* Page table flags for MMIO regions */
#define MMIO_FLAGS (PTE_PRESENT | PTE_WRITE | PTE_PWT | PTE_PCD)

/* PTE flags (from pagetable.h) */
#ifndef PTE_PRESENT
#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITE    (1ULL << 1)
#define PTE_USER     (1ULL << 2)
#define PTE_PWT      (1ULL << 3)  // Write-Through caching
#define PTE_PCD      (1ULL << 4)  // Cache Disable
#define PTE_ACCESSED (1ULL << 5)
#define PTE_DIRTY    (1ULL << 6)
#define PTE_PS       (1ULL << 7)  // Page Size (2MB/1GB pages)
#define PTE_GLOBAL   (1ULL << 8)
#endif

/**
 * Initialize MMIO subsystem
 * Must be called after page table initialization
 */
void mmio_init(void);

/**
 * Map a physical MMIO region into kernel virtual address space
 * 
 * @param phys_addr Physical address of MMIO region (must be page-aligned)
 * @param size Size of region in bytes (will be rounded up to page size)
 * @return Virtual address of mapped region, or NULL on error
 * 
 * Note: Mapped region is:
 * - Non-cacheable (PWT=1, PCD=1)
 * - Kernel-only (U=0)
 * - Read/Write (RW=1)
 */
void *mmio_map(uint64_t phys_addr, size_t size);

/**
 * Unmap a previously mapped MMIO region
 * 
 * @param virt_addr Virtual address returned by mmio_map()
 * @param size Size of region (same as passed to mmio_map)
 */
void mmio_unmap(void *virt_addr, size_t size);

/**
 * Check if a virtual address is in MMIO range
 */
int mmio_is_mmio_addr(uint64_t virt_addr);

/**
 * Get stats about MMIO usage
 */
typedef struct {
    uint64_t total_mappings;
    uint64_t total_size;
    uint64_t free_space;
} mmio_stats_t;

void mmio_get_stats(mmio_stats_t *stats);

#endif /* MMIO_H */
