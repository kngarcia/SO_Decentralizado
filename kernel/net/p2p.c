#include "p2p.h"
#include "udp.h"
#include "netif.h"
#include "../drivers/serial.h"
#include <stddef.h>

static p2p_network_t p2p_net;
static uint32_t time_counter = 0;

static uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

static uint32_t htonl(uint32_t val) {
    return ((val & 0xFF) << 24) | 
           (((val >> 8) & 0xFF) << 16) |
           (((val >> 16) & 0xFF) << 8) |
           ((val >> 24) & 0xFF);
}

int p2p_init(uint32_t node_id) {
    p2p_net.node_id = node_id;
    p2p_net.on_data = NULL;
    
    // Initialize peer table
    for (int i = 0; i < MAX_PEERS; i++) {
        p2p_net.peers[i].active = 0;
    }
    
    // Create UDP socket
    p2p_net.socket_fd = udp_socket();
    if (p2p_net.socket_fd < 0) {
        serial_puts("[p2p] ERROR: Failed to create socket\n");
        return -1;
    }
    
    // Bind to P2P port
    if (udp_bind(p2p_net.socket_fd, P2P_PORT) < 0) {
        serial_puts("[p2p] ERROR: Failed to bind to port\n");
        return -1;
    }
    
    serial_puts("[p2p] Initialized node ID: ");
    serial_put_hex(node_id);
    serial_putc('\n');
    
    return 0;
}

int p2p_add_peer(const ip_addr_t *ip, uint16_t port) {
    if (!ip) return -1;
    
    // Check if peer already exists
    for (int i = 0; i < MAX_PEERS; i++) {
        if (p2p_net.peers[i].active && 
            ip_addr_cmp(&p2p_net.peers[i].ip, ip) == 0 &&
            p2p_net.peers[i].port == port) {
            // Update last seen
            p2p_net.peers[i].last_seen = time_counter;
            return i;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_PEERS; i++) {
        if (!p2p_net.peers[i].active) {
            ip_addr_copy(&p2p_net.peers[i].ip, ip);
            p2p_net.peers[i].port = port;
            p2p_net.peers[i].node_id = 0; // Will be filled on first message
            p2p_net.peers[i].last_seen = time_counter;
            p2p_net.peers[i].active = 1;
            
            serial_puts("[p2p] Added peer: ");
            for (int j = 0; j < 4; j++) {
                serial_put_hex(ip->addr[j]);
                if (j < 3) serial_putc('.');
            }
            serial_putc('\n');
            
            return i;
        }
    }
    
    return -1; // No free slots
}

static int p2p_send_message(const ip_addr_t *dest_ip, uint16_t dest_port, 
                            uint8_t msg_type, const void *payload, uint16_t payload_len) {
    if (payload_len > MAX_P2P_MSG_LEN) return -1;
    
    uint8_t packet[sizeof(p2p_header_t) + MAX_P2P_MSG_LEN];
    p2p_header_t *hdr = (p2p_header_t *)packet;
    
    hdr->type = msg_type;
    hdr->version = 1;
    hdr->length = htons(payload_len);
    hdr->sender_id = htonl(p2p_net.node_id);
    
    // Copy payload
    if (payload && payload_len > 0) {
        for (uint16_t i = 0; i < payload_len; i++) {
            packet[sizeof(p2p_header_t) + i] = ((uint8_t *)payload)[i];
        }
    }
    
    return udp_sendto(p2p_net.socket_fd, packet, 
                      sizeof(p2p_header_t) + payload_len, 
                      dest_ip, dest_port);
}

int p2p_broadcast(const void *data, uint16_t len) {
    if (!data || len == 0) return -1;
    
    int sent = 0;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (p2p_net.peers[i].active) {
            if (p2p_send_message(&p2p_net.peers[i].ip, p2p_net.peers[i].port,
                                 P2P_MSG_DATA, data, len) >= 0) {
                sent++;
            }
        }
    }
    
    serial_puts("[p2p] Broadcast to ");
    serial_put_hex(sent);
    serial_puts(" peers\n");
    
    return sent;
}

int p2p_send_to_peer(uint32_t peer_id, const void *data, uint16_t len) {
    if (!data || len == 0) return -1;
    
    // Find peer by node_id
    for (int i = 0; i < MAX_PEERS; i++) {
        if (p2p_net.peers[i].active && p2p_net.peers[i].node_id == peer_id) {
            return p2p_send_message(&p2p_net.peers[i].ip, p2p_net.peers[i].port,
                                    P2P_MSG_DATA, data, len);
        }
    }
    
    return -1;
}

int p2p_discover_peers(void) {
    // Send announce message to broadcast/multicast
    // For simplicity, send to all known peers
    serial_puts("[p2p] Sending peer discovery\n");
    
    for (int i = 0; i < MAX_PEERS; i++) {
        if (p2p_net.peers[i].active) {
            p2p_send_message(&p2p_net.peers[i].ip, p2p_net.peers[i].port,
                            P2P_MSG_ANNOUNCE, NULL, 0);
        }
    }
    
    return 0;
}

void p2p_process_packet(const void *data, uint32_t len, const ip_addr_t *src_ip, uint16_t src_port) {
    if (!data || len < sizeof(p2p_header_t)) return;
    
    const p2p_header_t *hdr = (const p2p_header_t *)data;
    const uint8_t *payload = (const uint8_t *)data + sizeof(p2p_header_t);
    uint16_t payload_len = htons(hdr->length);
    uint32_t sender_id = htonl(hdr->sender_id);
    
    time_counter++;
    
    // Update or add peer
    int peer_idx = -1;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (p2p_net.peers[i].active && 
            ip_addr_cmp(&p2p_net.peers[i].ip, src_ip) == 0) {
            peer_idx = i;
            p2p_net.peers[i].node_id = sender_id;
            p2p_net.peers[i].last_seen = time_counter;
            break;
        }
    }
    
    if (peer_idx < 0) {
        // New peer
        p2p_add_peer(src_ip, src_port);
    }
    
    // Process message based on type
    switch (hdr->type) {
        case P2P_MSG_PING:
            serial_puts("[p2p] Received PING from ");
            serial_put_hex(sender_id);
            serial_putc('\n');
            // Send PONG
            p2p_send_message(src_ip, src_port, P2P_MSG_PONG, NULL, 0);
            break;
            
        case P2P_MSG_PONG:
            serial_puts("[p2p] Received PONG from ");
            serial_put_hex(sender_id);
            serial_putc('\n');
            break;
            
        case P2P_MSG_ANNOUNCE:
            serial_puts("[p2p] Received ANNOUNCE from ");
            serial_put_hex(sender_id);
            serial_putc('\n');
            // Send our own announce back
            p2p_send_message(src_ip, src_port, P2P_MSG_ANNOUNCE, NULL, 0);
            break;
            
        case P2P_MSG_DATA:
            serial_puts("[p2p] Received DATA from ");
            serial_put_hex(sender_id);
            serial_puts(" len=");
            serial_put_hex(payload_len);
            serial_putc('\n');
            
            // Call data handler if set
            if (p2p_net.on_data) {
                p2p_net.on_data(sender_id, payload, payload_len);
            }
            break;
            
        default:
            serial_puts("[p2p] Unknown message type\n");
            break;
    }
}

void p2p_set_data_handler(void (*handler)(uint32_t sender_id, const void *data, uint16_t len)) {
    p2p_net.on_data = handler;
}

p2p_peer_t* p2p_get_peers(int *count) {
    if (count) {
        *count = 0;
        for (int i = 0; i < MAX_PEERS; i++) {
            if (p2p_net.peers[i].active) (*count)++;
        }
    }
    return p2p_net.peers;
}
