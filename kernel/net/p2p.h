#ifndef P2P_H
#define P2P_H

#include <stdint.h>
#include "netif.h"

#define P2P_PORT 6000
#define MAX_PEERS 16
#define MAX_P2P_MSG_LEN 1400

// P2P message types
#define P2P_MSG_PING       1
#define P2P_MSG_PONG       2
#define P2P_MSG_ANNOUNCE   3
#define P2P_MSG_DISCOVER   4
#define P2P_MSG_DATA       5

typedef struct {
    uint8_t type;
    uint8_t version;
    uint16_t length;
    uint32_t sender_id;
    uint8_t payload[0];
} __attribute__((packed)) p2p_header_t;

typedef struct {
    ip_addr_t ip;
    uint32_t node_id;
    uint16_t port;
    uint32_t last_seen;  // Timestamp
    uint8_t active;
} p2p_peer_t;

typedef struct {
    uint32_t node_id;
    p2p_peer_t peers[MAX_PEERS];
    int socket_fd;
    void (*on_data)(uint32_t sender_id, const void *data, uint16_t len);
} p2p_network_t;

// P2P functions
int p2p_init(uint32_t node_id);
int p2p_add_peer(const ip_addr_t *ip, uint16_t port);
int p2p_broadcast(const void *data, uint16_t len);
int p2p_send_to_peer(uint32_t peer_id, const void *data, uint16_t len);
void p2p_process_packet(const void *data, uint32_t len, const ip_addr_t *src_ip, uint16_t src_port);
int p2p_discover_peers(void);
void p2p_set_data_handler(void (*handler)(uint32_t sender_id, const void *data, uint16_t len));
p2p_peer_t* p2p_get_peers(int *count);

#endif // P2P_H
