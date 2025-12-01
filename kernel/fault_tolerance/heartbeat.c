/* kernel/fault_tolerance/heartbeat.c
 * Fault tolerance: heartbeat monitoring and failure detection
 */

#include <stdint.h>
#include <string.h>
#include "../drivers/serial.h"

#define MAX_MONITORED_NODES 32
#define HEARTBEAT_INTERVAL 1000     /* 1 second */
#define FAILURE_THRESHOLD 5000      /* 5 seconds without heartbeat */

typedef struct {
    uint32_t node_id;
    uint32_t last_heartbeat;
    uint32_t missed_beats;
    int active;
    int failed;
} node_monitor_t;

static node_monitor_t monitored_nodes[MAX_MONITORED_NODES];
static uint32_t local_node_id = 1;
static uint32_t current_time = 0;
static uint32_t coordinator_node = 1;
static int ft_initialized = 0;

/* Initialize fault tolerance */
int ft_init(uint32_t node_id) {
    local_node_id = node_id;
    coordinator_node = node_id;  /* Initially we are coordinator */
    current_time = 0;
    
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        monitored_nodes[i].active = 0;
        monitored_nodes[i].failed = 0;
    }
    
    /* Register self */
    monitored_nodes[0].node_id = node_id;
    monitored_nodes[0].active = 1;
    monitored_nodes[0].failed = 0;
    monitored_nodes[0].last_heartbeat = current_time;
    monitored_nodes[0].missed_beats = 0;
    
    ft_initialized = 1;
    
    serial_puts("[ft] Fault tolerance initialized for node ");
    serial_put_hex(node_id);
    serial_putc('\n');
    
    return 0;
}

/* Register node for monitoring */
int ft_register_node(uint32_t node_id) {
    /* Find existing or free slot */
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (monitored_nodes[i].active && monitored_nodes[i].node_id == node_id) {
            /* Already registered */
            return i;
        }
    }
    
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (!monitored_nodes[i].active) {
            monitored_nodes[i].node_id = node_id;
            monitored_nodes[i].active = 1;
            monitored_nodes[i].failed = 0;
            monitored_nodes[i].last_heartbeat = current_time;
            monitored_nodes[i].missed_beats = 0;
            
            serial_puts("[ft] Registered node ");
            serial_put_hex(node_id);
            serial_puts(" for monitoring\n");
            
            return i;
        }
    }
    
    return -1;
}

/* Send heartbeat to all peers */
int ft_send_heartbeat(void) {
    if (!ft_initialized) return -1;
    
    /* Build heartbeat packet */
    typedef struct {
        uint8_t type;       /* 0x30 = HEARTBEAT */
        uint32_t sender;
        uint32_t timestamp;
        uint32_t coordinator;  /* Current coordinator */
    } heartbeat_packet_t;
    
    heartbeat_packet_t hb;
    hb.type = 0x30;
    hb.sender = local_node_id;
    hb.timestamp = current_time;
    hb.coordinator = coordinator_node;
    
    /* Broadcast heartbeat (stub) */
    serial_puts("[ft] Sending heartbeat (timestamp=");
    serial_put_hex(current_time);
    serial_puts(")\n");
    
    return 0;
}

/* Receive heartbeat from peer */
int ft_receive_heartbeat(uint32_t sender, uint32_t timestamp) {
    /* Update clock */
    if (timestamp > current_time) {
        current_time = timestamp;
    }
    
    /* Find node */
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (monitored_nodes[i].active && monitored_nodes[i].node_id == sender) {
            monitored_nodes[i].last_heartbeat = current_time;
            monitored_nodes[i].missed_beats = 0;
            
            if (monitored_nodes[i].failed) {
                serial_puts("[ft] Node ");
                serial_put_hex(sender);
                serial_puts(" recovered!\n");
                monitored_nodes[i].failed = 0;
            }
            
            return 0;
        }
    }
    
    /* Unknown node - register it */
    ft_register_node(sender);
    return 0;
}

/* Monitor all peers for failures */
int ft_monitor_peers(void) {
    if (!ft_initialized) return -1;
    
    current_time++;
    
    int failures_detected = 0;
    
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (!monitored_nodes[i].active || monitored_nodes[i].node_id == local_node_id) {
            continue;
        }
        
        uint32_t time_since_last = current_time - monitored_nodes[i].last_heartbeat;
        
        if (time_since_last > FAILURE_THRESHOLD && !monitored_nodes[i].failed) {
            /* Node failure detected */
            monitored_nodes[i].failed = 1;
            monitored_nodes[i].missed_beats++;
            
            serial_puts("[ft] FAILURE DETECTED: Node ");
            serial_put_hex(monitored_nodes[i].node_id);
            serial_puts(" (last seen ");
            serial_put_hex(time_since_last);
            serial_puts(" ticks ago)\n");
            
            failures_detected++;
            
            /* Handle failure */
            ft_handle_node_failure(monitored_nodes[i].node_id);
        }
    }
    
    return failures_detected;
}

/* Handle node failure */
int ft_handle_node_failure(uint32_t failed_node) {
    serial_puts("[ft] Handling failure of node ");
    serial_put_hex(failed_node);
    serial_putc('\n');
    
    /* If failed node was coordinator, trigger election */
    if (failed_node == coordinator_node) {
        serial_puts("[ft] Coordinator failed! Triggering election...\n");
        ft_elect_coordinator();
    }
    
    /* Notify distributed scheduler */
    extern int dsched_update_node_load(uint32_t node_id, uint32_t load);
    dsched_update_node_load(failed_node, 0);
    
    /* Invalidate DSM regions owned by failed node */
    serial_puts("[ft] Invalidating resources owned by failed node\n");
    
    return 0;
}

/* Elect new coordinator (Bully algorithm) */
int ft_elect_coordinator(void) {
    serial_puts("[ft] Starting coordinator election\n");
    
    /* Find highest node ID that is not failed */
    uint32_t highest_id = 0;
    
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (monitored_nodes[i].active && !monitored_nodes[i].failed) {
            if (monitored_nodes[i].node_id > highest_id) {
                highest_id = monitored_nodes[i].node_id;
            }
        }
    }
    
    if (highest_id == 0) {
        serial_puts("[ft] ERROR: No active nodes found\n");
        return -1;
    }
    
    /* If we are the highest ID, we become coordinator */
    if (highest_id == local_node_id) {
        coordinator_node = local_node_id;
        serial_puts("[ft] Elected as coordinator (node ");
        serial_put_hex(local_node_id);
        serial_puts(")\n");
        
        /* Broadcast COORDINATOR message */
        ft_announce_coordinator();
    } else {
        coordinator_node = highest_id;
        serial_puts("[ft] Node ");
        serial_put_hex(highest_id);
        serial_puts(" elected as coordinator\n");
    }
    
    return 0;
}

/* Announce coordinator */
int ft_announce_coordinator(void) {
    typedef struct {
        uint8_t type;       /* 0x31 = COORDINATOR */
        uint32_t coordinator;
    } coordinator_packet_t;
    
    coordinator_packet_t pkt;
    pkt.type = 0x31;
    pkt.coordinator = local_node_id;
    
    /* Broadcast (stub) */
    serial_puts("[ft] Broadcasting coordinator announcement\n");
    
    return 0;
}

/* Get current coordinator */
uint32_t ft_get_coordinator(void) {
    return coordinator_node;
}

/* Check if node is alive */
int ft_is_node_alive(uint32_t node_id) {
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (monitored_nodes[i].active && monitored_nodes[i].node_id == node_id) {
            return !monitored_nodes[i].failed;
        }
    }
    return 0;
}

/* Print fault tolerance statistics */
void ft_print_stats(void) {
    serial_puts("\n[ft] === Fault Tolerance Statistics ===\n");
    serial_puts("[ft] Coordinator: Node ");
    serial_put_hex(coordinator_node);
    serial_putc('\n');
    serial_puts("[ft] Current time: ");
    serial_put_hex(current_time);
    serial_putc('\n');
    
    int active_count = 0;
    int failed_count = 0;
    
    for (int i = 0; i < MAX_MONITORED_NODES; i++) {
        if (monitored_nodes[i].active) {
            active_count++;
            
            serial_puts("  Node ");
            serial_put_hex(monitored_nodes[i].node_id);
            serial_puts(": ");
            
            if (monitored_nodes[i].failed) {
                serial_puts("FAILED");
                failed_count++;
            } else {
                serial_puts("ALIVE (last_hb=");
                serial_put_hex(monitored_nodes[i].last_heartbeat);
                serial_putc(')');
            }
            
            if (monitored_nodes[i].node_id == local_node_id) {
                serial_puts(" (local)");
            }
            if (monitored_nodes[i].node_id == coordinator_node) {
                serial_puts(" (coordinator)");
            }
            serial_putc('\n');
        }
    }
    
    serial_puts("[ft] Active nodes: ");
    serial_put_hex(active_count);
    serial_puts("\n[ft] Failed nodes: ");
    serial_put_hex(failed_count);
    serial_puts("\n[ft] =====================================\n\n");
}
