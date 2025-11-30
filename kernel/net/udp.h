#ifndef UDP_H
#define UDP_H

#include <stdint.h>
#include "netif.h"

#define UDP_HDR_LEN 8
#define MAX_UDP_SOCKETS 16

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} __attribute__((packed)) udp_hdr_t;

typedef struct {
    uint16_t local_port;
    uint8_t in_use;
    
    // Receive queue (simple single packet buffer for now)
    uint8_t rx_buf[1500];
    uint16_t rx_len;
    ip_addr_t rx_src_ip;
    uint16_t rx_src_port;
    uint8_t rx_ready;
} udp_socket_t;

// UDP functions
int udp_init(void);
int udp_socket(void);
int udp_bind(int sockfd, uint16_t port);
int udp_sendto(int sockfd, const void *data, uint16_t len, const ip_addr_t *dest_ip, uint16_t dest_port);
int udp_recvfrom(int sockfd, void *buf, uint16_t max_len, ip_addr_t *src_ip, uint16_t *src_port);
void udp_input(netif_t *netif, const ip_addr_t *src_ip, const void *data, uint32_t len);
int udp_close(int sockfd);

#endif // UDP_H
