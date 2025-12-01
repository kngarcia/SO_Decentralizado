/* kernel/fault_tolerance/heartbeat.h */
#ifndef FAULT_TOLERANCE_HEARTBEAT_H
#define FAULT_TOLERANCE_HEARTBEAT_H

#include <stdint.h>

/* Initialize fault tolerance */
int ft_init(uint32_t node_id);

/* Register node for monitoring */
int ft_register_node(uint32_t node_id);

/* Send heartbeat to peers */
int ft_send_heartbeat(void);

/* Receive heartbeat from peer */
int ft_receive_heartbeat(uint32_t sender, uint32_t timestamp);

/* Monitor all peers for failures */
int ft_monitor_peers(void);

/* Handle node failure */
int ft_handle_node_failure(uint32_t failed_node);

/* Elect new coordinator */
int ft_elect_coordinator(void);

/* Announce coordinator */
int ft_announce_coordinator(void);

/* Get current coordinator */
uint32_t ft_get_coordinator(void);

/* Check if node is alive */
int ft_is_node_alive(uint32_t node_id);

/* Print statistics */
void ft_print_stats(void);

#endif /* FAULT_TOLERANCE_HEARTBEAT_H */
