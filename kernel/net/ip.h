#ifndef IP_H
#define IP_H

#include <stdint.h>
#include "netif.h"

#define IP_PROTO_ICMP  1
#define IP_PROTO_TCP   6
#define IP_PROTO_UDP   17

#define IP_HDR_LEN     20

typedef struct {
    uint8_t version_ihl;    // Version (4 bits) + IHL (4 bits)
    uint8_t tos;            // Type of service
    uint16_t len;           // Total length
    uint16_t id;            // Identification
    uint16_t frag_off;      // Fragment offset
    uint8_t ttl;            // Time to live
    uint8_t proto;          // Protocol
    uint16_t checksum;      // Header checksum
    ip_addr_t src;          // Source IP
    ip_addr_t dst;          // Destination IP
} __attribute__((packed)) ip_hdr_t;

// IP layer functions
int ip_init(void);
int ip_send(netif_t *netif, const ip_addr_t *dest, uint8_t proto, const void *data, uint16_t len);
void ip_input(netif_t *netif, const void *data, uint32_t len);
uint16_t ip_checksum(const void *data, uint16_t len);

#endif // IP_H
