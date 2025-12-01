/* kernel/mm/distributed_memory.c
 * Distributed Shared Memory (DSM) implementation
 */

#include <stdint.h>
#include <string.h>
#include "../drivers/serial.h"

#define DSM_MAX_REGIONS 64
#define DSM_PAGE_SIZE 4096

typedef struct {
    uint64_t vaddr;
    uint64_t size;
    uint32_t owner_node;
    int replicas;
    int active;
    uint8_t state;  /* 0=invalid, 1=read-only, 2=read-write */
} dsm_region_t;

static dsm_region_t dsm_regions[DSM_MAX_REGIONS];
static uint32_t local_node_id = 1;
static int dsm_initialized = 0;

/* Initialize DSM subsystem */
int dsm_init(uint32_t node_id) {
    local_node_id = node_id;
    
    for (int i = 0; i < DSM_MAX_REGIONS; i++) {
        dsm_regions[i].active = 0;
        dsm_regions[i].state = 0;
    }
    
    dsm_initialized = 1;
    
    serial_puts("[dsm] Distributed Shared Memory initialized for node ");
    serial_put_hex(node_id);
    serial_putc('\n');
    
    return 0;
}

/* Allocate distributed shared memory region */
void *dsm_alloc(uint64_t size, int replicas) {
    if (!dsm_initialized) return NULL;
    
    /* Find free region slot */
    int slot = -1;
    for (int i = 0; i < DSM_MAX_REGIONS; i++) {
        if (!dsm_regions[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        serial_puts("[dsm] ERROR: No free DSM region slots\n");
        return NULL;
    }
    
    /* Allocate virtual memory for the region */
    extern void *virtual_memory_alloc(uint64_t vaddr, uint64_t size, int prot);
    void *vaddr = virtual_memory_alloc(0, size, 0);
    
    if (!vaddr) {
        serial_puts("[dsm] ERROR: Failed to allocate virtual memory\n");
        return NULL;
    }
    
    /* Initialize DSM region */
    dsm_regions[slot].vaddr = (uint64_t)vaddr;
    dsm_regions[slot].size = size;
    dsm_regions[slot].owner_node = local_node_id;
    dsm_regions[slot].replicas = replicas;
    dsm_regions[slot].active = 1;
    dsm_regions[slot].state = 2;  /* read-write (we're owner) */
    
    serial_puts("[dsm] Allocated DSM region at vaddr=");
    serial_put_hex((uint64_t)vaddr);
    serial_puts(" size=");
    serial_put_hex(size);
    serial_puts(" replicas=");
    serial_put_hex(replicas);
    serial_putc('\n');
    
    return vaddr;
}

/* Free distributed shared memory region */
void dsm_free(void *vaddr) {
    uint64_t addr = (uint64_t)vaddr;
    
    for (int i = 0; i < DSM_MAX_REGIONS; i++) {
        if (dsm_regions[i].active && dsm_regions[i].vaddr == addr) {
            dsm_regions[i].active = 0;
            
            serial_puts("[dsm] Freed DSM region at vaddr=");
            serial_put_hex(addr);
            serial_putc('\n');
            
            /* Send invalidation message to replicas (stub) */
            
            return;
        }
    }
    
    serial_puts("[dsm] WARNING: Attempted to free unknown DSM region\n");
}

/* Synchronize page with owner node */
int dsm_sync_page(uint64_t vaddr, uint32_t owner_node) {
    serial_puts("[dsm] Syncing page vaddr=");
    serial_put_hex(vaddr);
    serial_puts(" with owner node=");
    serial_put_hex(owner_node);
    serial_putc('\n');
    
    /* Find DSM region containing this address */
    dsm_region_t *region = NULL;
    for (int i = 0; i < DSM_MAX_REGIONS; i++) {
        if (dsm_regions[i].active && 
            vaddr >= dsm_regions[i].vaddr &&
            vaddr < dsm_regions[i].vaddr + dsm_regions[i].size) {
            region = &dsm_regions[i];
            break;
        }
    }
    
    if (!region) {
        serial_puts("[dsm] ERROR: Address not in any DSM region\n");
        return -1;
    }
    
    if (owner_node == local_node_id) {
        /* We are the owner, no sync needed */
        return 0;
    }
    
    /* Build sync request packet */
    typedef struct {
        uint8_t type;       /* 0x10 = DSM_SYNC_REQUEST */
        uint32_t requester;
        uint32_t owner;
        uint64_t vaddr;
        uint64_t size;
    } dsm_sync_packet_t;
    
    dsm_sync_packet_t pkt;
    pkt.type = 0x10;
    pkt.requester = local_node_id;
    pkt.owner = owner_node;
    pkt.vaddr = vaddr & ~(DSM_PAGE_SIZE - 1);  /* Page-aligned */
    pkt.size = DSM_PAGE_SIZE;
    
    /* Send via network (stub) */
    serial_puts("[dsm] Sync request sent (stub)\n");
    
    /* Mark page as read-only until we receive update */
    region->state = 1;
    
    return 0;
}

/* Handle remote page fault */
int dsm_handle_remote_fault(uint64_t vaddr, uint32_t faulting_node) {
    serial_puts("[dsm] Remote page fault: vaddr=");
    serial_put_hex(vaddr);
    serial_puts(" from node=");
    serial_put_hex(faulting_node);
    serial_putc('\n');
    
    /* Find DSM region */
    dsm_region_t *region = NULL;
    for (int i = 0; i < DSM_MAX_REGIONS; i++) {
        if (dsm_regions[i].active && 
            vaddr >= dsm_regions[i].vaddr &&
            vaddr < dsm_regions[i].vaddr + dsm_regions[i].size) {
            region = &dsm_regions[i];
            break;
        }
    }
    
    if (!region) {
        serial_puts("[dsm] ERROR: Address not in DSM region\n");
        return -1;
    }
    
    if (region->owner_node != local_node_id) {
        serial_puts("[dsm] ERROR: We are not the owner\n");
        return -1;
    }
    
    /* Send page data to requesting node */
    uint64_t page_addr = vaddr & ~(DSM_PAGE_SIZE - 1);
    
    typedef struct {
        uint8_t type;       /* 0x11 = DSM_PAGE_DATA */
        uint32_t owner;
        uint64_t vaddr;
        uint64_t size;
        uint8_t data[DSM_PAGE_SIZE];
    } dsm_page_packet_t;
    
    /* Build and send page packet (stub) */
    serial_puts("[dsm] Sending page data to node ");
    serial_put_hex(faulting_node);
    serial_putc('\n');
    
    return 0;
}

/* Invalidate page on all nodes */
int dsm_invalidate_page(uint64_t vaddr) {
    serial_puts("[dsm] Invalidating page vaddr=");
    serial_put_hex(vaddr);
    serial_putc('\n');
    
    /* Send invalidation to all nodes with replicas */
    typedef struct {
        uint8_t type;       /* 0x12 = DSM_INVALIDATE */
        uint32_t owner;
        uint64_t vaddr;
    } dsm_invalidate_packet_t;
    
    dsm_invalidate_packet_t pkt;
    pkt.type = 0x12;
    pkt.owner = local_node_id;
    pkt.vaddr = vaddr & ~(DSM_PAGE_SIZE - 1);
    
    /* Broadcast invalidation (stub) */
    serial_puts("[dsm] Invalidation broadcast (stub)\n");
    
    return 0;
}

/* Get DSM statistics */
void dsm_print_stats(void) {
    serial_puts("\n[dsm] === DSM Statistics ===\n");
    
    int active_count = 0;
    uint64_t total_size = 0;
    
    for (int i = 0; i < DSM_MAX_REGIONS; i++) {
        if (dsm_regions[i].active) {
            active_count++;
            total_size += dsm_regions[i].size;
            
            serial_puts("  Region ");
            serial_put_hex(i);
            serial_puts(": vaddr=");
            serial_put_hex(dsm_regions[i].vaddr);
            serial_puts(" size=");
            serial_put_hex(dsm_regions[i].size);
            serial_puts(" owner=");
            serial_put_hex(dsm_regions[i].owner_node);
            serial_putc('\n');
        }
    }
    
    serial_puts("[dsm] Total regions: ");
    serial_put_hex(active_count);
    serial_puts("\n[dsm] Total size: ");
    serial_put_hex(total_size);
    serial_puts(" bytes\n");
    serial_puts("[dsm] =========================\n\n");
}
