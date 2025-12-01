/* kernel/scheduler/distributed.c
 * Distributed scheduler for process migration and load balancing
 */

#include "../process_manager.h"
#include "../drivers/serial.h"
#include "../net/p2p.h"
#include <stdint.h>
#include <string.h>

#define MAX_NODES 32
#define MIGRATION_THRESHOLD 0.7  /* 70% load triggers migration */

typedef struct {
    uint32_t node_id;
    uint32_t load;        /* Number of processes */
    uint32_t last_seen;   /* Heartbeat timestamp */
    int active;
} node_info_t;

static node_info_t cluster_nodes[MAX_NODES];
static uint32_t local_node_id = 1;
static int distributed_enabled = 0;

/* Initialize distributed scheduler */
int dsched_init(uint32_t node_id) {
    local_node_id = node_id;
    
    for (int i = 0; i < MAX_NODES; i++) {
        cluster_nodes[i].active = 0;
        cluster_nodes[i].load = 0;
    }
    
    /* Register local node */
    cluster_nodes[0].node_id = local_node_id;
    cluster_nodes[0].active = 1;
    cluster_nodes[0].load = 0;
    
    distributed_enabled = 1;
    
    serial_puts("[dsched] Distributed scheduler initialized for node ");
    serial_put_hex(node_id);
    serial_putc('\n');
    
    return 0;
}

/* Register a remote node in the cluster */
int dsched_register_node(uint32_t node_id) {
    /* Find existing or free slot */
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].active && cluster_nodes[i].node_id == node_id) {
            /* Already registered */
            return i;
        }
    }
    
    for (int i = 0; i < MAX_NODES; i++) {
        if (!cluster_nodes[i].active) {
            cluster_nodes[i].node_id = node_id;
            cluster_nodes[i].active = 1;
            cluster_nodes[i].load = 0;
            
            serial_puts("[dsched] Registered remote node ");
            serial_put_hex(node_id);
            serial_putc('\n');
            
            return i;
        }
    }
    
    return -1;  /* No free slots */
}

/* Get load of a specific node */
uint32_t dsched_get_node_load(uint32_t node_id) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].active && cluster_nodes[i].node_id == node_id) {
            return cluster_nodes[i].load;
        }
    }
    return 0;
}

/* Find node with lowest load */
static uint32_t find_least_loaded_node(void) {
    uint32_t min_load = 0xFFFFFFFF;
    uint32_t target_node = local_node_id;
    
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].active && cluster_nodes[i].load < min_load) {
            min_load = cluster_nodes[i].load;
            target_node = cluster_nodes[i].node_id;
        }
    }
    
    return target_node;
}

/* Migrate process to remote node */
int dsched_migrate_process(process_t *proc, uint32_t dest_node_id) {
    if (!distributed_enabled || !proc) return -1;
    
    if (dest_node_id == local_node_id) {
        serial_puts("[dsched] Cannot migrate to local node\n");
        return -1;
    }
    
    serial_puts("[dsched] Migrating process PID=");
    serial_put_hex(proc->pid);
    serial_puts(" to node ");
    serial_put_hex(dest_node_id);
    serial_putc('\n');
    
    /* Build migration packet:
     * - Process state (registers, stack, heap)
     * - Page table mappings
     * - File descriptors
     */
    
    /* Serialize process state */
    typedef struct {
        uint8_t type;        /* 0x01 = MIGRATION_REQUEST */
        uint32_t src_node;
        uint32_t dest_node;
        uint64_t pid;
        uint64_t entry_point;
        uint64_t stack_top;
        uint64_t stack_size;
        uint64_t heap_start;
        uint64_t heap_end;
        int state;
    } migration_packet_t;
    
    migration_packet_t pkt;
    pkt.type = 0x01;
    pkt.src_node = local_node_id;
    pkt.dest_node = dest_node_id;
    pkt.pid = proc->pid;
    pkt.entry_point = proc->entry_point;
    pkt.stack_top = proc->stack_top;
    pkt.stack_size = proc->stack_size;
    pkt.heap_start = proc->heap_start;
    pkt.heap_end = proc->heap_end;
    pkt.state = proc->state;
    
    /* Send via P2P network (stub for now) */
    extern int p2p_send_to_node(uint32_t node_id, const void *data, uint16_t len);
    // p2p_send_to_node(dest_node_id, &pkt, sizeof(pkt));
    
    serial_puts("[dsched] Migration packet sent (stub)\n");
    
    /* Update load counters */
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].node_id == local_node_id) {
            if (cluster_nodes[i].load > 0) cluster_nodes[i].load--;
        }
        if (cluster_nodes[i].node_id == dest_node_id) {
            cluster_nodes[i].load++;
        }
    }
    
    return 0;
}

/* Request a remote task from another node */
int dsched_request_remote_task(uint32_t node_id) {
    serial_puts("[dsched] Requesting task from node ");
    serial_put_hex(node_id);
    serial_putc('\n');
    
    /* Send TASK_REQUEST packet */
    uint8_t request[8];
    request[0] = 0x02;  /* TASK_REQUEST */
    request[1] = (local_node_id >> 24) & 0xFF;
    request[2] = (local_node_id >> 16) & 0xFF;
    request[3] = (local_node_id >> 8) & 0xFF;
    request[4] = local_node_id & 0xFF;
    
    /* Send via P2P (stub) */
    
    return 0;
}

/* Load balancing algorithm */
int dsched_balance_load(void) {
    if (!distributed_enabled) return -1;
    
    /* Get local load */
    uint32_t local_load = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].node_id == local_node_id) {
            local_load = cluster_nodes[i].load;
            break;
        }
    }
    
    /* Find average cluster load */
    uint32_t total_load = 0;
    int active_nodes = 0;
    
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].active) {
            total_load += cluster_nodes[i].load;
            active_nodes++;
        }
    }
    
    if (active_nodes == 0) return 0;
    
    uint32_t avg_load = total_load / active_nodes;
    
    serial_puts("[dsched] Load balance: local=");
    serial_put_hex(local_load);
    serial_puts(" avg=");
    serial_put_hex(avg_load);
    serial_putc('\n');
    
    /* If local load > threshold * avg_load, migrate process */
    if (local_load > (uint32_t)(MIGRATION_THRESHOLD * avg_load + 2)) {
        uint32_t target = find_least_loaded_node();
        if (target != local_node_id) {
            serial_puts("[dsched] Load imbalance detected, triggering migration\n");
            /* Find a process to migrate (stub - would select from scheduler) */
            return 1;  /* Signal migration needed */
        }
    }
    
    return 0;
}

/* Update remote node load information */
int dsched_update_node_load(uint32_t node_id, uint32_t load) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].active && cluster_nodes[i].node_id == node_id) {
            cluster_nodes[i].load = load;
            return 0;
        }
    }
    return -1;
}

/* Get cluster statistics */
void dsched_print_cluster_stats(void) {
    serial_puts("\n[dsched] === Cluster Statistics ===\n");
    
    for (int i = 0; i < MAX_NODES; i++) {
        if (cluster_nodes[i].active) {
            serial_puts("  Node ");
            serial_put_hex(cluster_nodes[i].node_id);
            serial_puts(": load=");
            serial_put_hex(cluster_nodes[i].load);
            if (cluster_nodes[i].node_id == local_node_id) {
                serial_puts(" (local)");
            }
            serial_putc('\n');
        }
    }
    
    serial_puts("[dsched] ===========================\n\n");
}
