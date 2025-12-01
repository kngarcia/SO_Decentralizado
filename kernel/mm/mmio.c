/* kernel/mm/mmio.c
 * Memory-Mapped I/O (MMIO) management implementation
 */

#include "mmio.h"
#include "../mm/pagetable.h"
#include <stdint.h>
#include <stddef.h>

extern void show_string(const char *s);
extern void *memset(void *s, int c, size_t n);

/* MMIO allocation tracking */
#define MMIO_MAX_REGIONS 16

typedef struct {
    uint64_t virt_addr;
    uint64_t phys_addr;
    size_t size;
    int used;
} mmio_region_t;

static mmio_region_t mmio_regions[MMIO_MAX_REGIONS];
static uint64_t mmio_next_virt = MMIO_VIRT_BASE;
static int mmio_initialized = 0;

/* External page table functions */
extern uint64_t *get_kernel_pml4(void);
extern void pagetable_map(uint64_t *pml4, uint64_t virt, uint64_t phys, int flags);
extern void invlpg(uint64_t addr);

void mmio_init(void) {
    if (mmio_initialized) return;
    
    memset(mmio_regions, 0, sizeof(mmio_regions));
    mmio_next_virt = MMIO_VIRT_BASE;
    mmio_initialized = 1;
    
    show_string("[mmio] Initialized (range: 0xFFFF800000000000-0xFFFF880000000000)\n");
}

void *mmio_map(uint64_t phys_addr, size_t size) {
    if (!mmio_initialized) {
        mmio_init();
    }
    
    /* Align physical address and size to page boundaries */
    uint64_t phys_aligned = phys_addr & ~0xFFFULL;
    uint64_t offset = phys_addr & 0xFFFULL;
    size_t size_aligned = ((size + offset + 0xFFF) & ~0xFFFULL);
    
    /* Check size limit */
    if (size_aligned > MMIO_MAX_SIZE) {
        show_string("[mmio] ERROR: Region too large\n");
        return NULL;
    }
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MMIO_MAX_REGIONS; i++) {
        if (!mmio_regions[i].used) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        show_string("[mmio] ERROR: No free slots\n");
        return NULL;
    }
    
    /* SIMPLE SOLUTION: Use identity mapping (phys == virt)
     * The 4GB identity mapping in start.S using 2MB large pages
     * should cover all MMIO regions including E1000 @ 0xFEBC0000
     * We rely on the PS (Page Size) bit in PDE for 2MB pages
     */
    
    uint64_t virt_addr = phys_aligned;  /* Identity mapping */
    
    /* Record the mapping */
    mmio_regions[slot].virt_addr = virt_addr;
    mmio_regions[slot].phys_addr = phys_aligned;
    mmio_regions[slot].size = size_aligned;
    mmio_regions[slot].used = 1;
    
    /* Simple debug - avoid complex hex printing that might fault */
    show_string("[mmio] Mapped MMIO region\\n");
    
    return (void *)(virt_addr + offset);
}

void mmio_unmap(void *virt_addr, size_t size) {
    uint64_t virt = (uint64_t)virt_addr;
    
    /* Find matching region */
    for (int i = 0; i < MMIO_MAX_REGIONS; i++) {
        if (mmio_regions[i].used && 
            virt >= mmio_regions[i].virt_addr &&
            virt < mmio_regions[i].virt_addr + mmio_regions[i].size) {
            
            /* Mark as free */
            mmio_regions[i].used = 0;
            
            /* Note: We don't actually unmap the pages to avoid complexity
             * In a production system, you would:
             * 1. Clear page table entries
             * 2. Invalidate TLB
             * 3. Potentially coalesce free virtual address space
             */
            
            show_string("[mmio] Unmapped region at 0x");
            extern void show_hex(uint64_t val);
            show_hex(virt);
            show_string("\n");
            return;
        }
    }
    
    show_string("[mmio] WARNING: Region not found for unmap\n");
}

int mmio_is_mmio_addr(uint64_t virt_addr) {
    return (virt_addr >= MMIO_VIRT_BASE && virt_addr < MMIO_VIRT_END);
}

void mmio_get_stats(mmio_stats_t *stats) {
    stats->total_mappings = 0;
    stats->total_size = 0;
    
    for (int i = 0; i < MMIO_MAX_REGIONS; i++) {
        if (mmio_regions[i].used) {
            stats->total_mappings++;
            stats->total_size += mmio_regions[i].size;
        }
    }
    
    stats->free_space = (MMIO_VIRT_END - mmio_next_virt);
}
