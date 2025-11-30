#ifndef MDNS_H
#define MDNS_H

#include <stdint.h>
#include "netif.h"

#define MDNS_PORT 5353
#define MDNS_TTL  120

// mDNS multicast address: 224.0.0.251
#define MDNS_MCAST_IP_0 224
#define MDNS_MCAST_IP_1 0
#define MDNS_MCAST_IP_2 0
#define MDNS_MCAST_IP_3 251

// DNS header flags
#define DNS_FLAG_RESPONSE    0x8000
#define DNS_FLAG_AUTHORITATIVE 0x0400

// DNS record types
#define DNS_TYPE_A     1
#define DNS_TYPE_PTR   12
#define DNS_TYPE_TXT   16
#define DNS_TYPE_SRV   33

#define MAX_SERVICES 8
#define MAX_SERVICE_NAME_LEN 64

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;  // Question count
    uint16_t ancount;  // Answer count
    uint16_t nscount;  // Authority count
    uint16_t arcount;  // Additional count
} __attribute__((packed)) dns_header_t;

typedef struct {
    char service_name[MAX_SERVICE_NAME_LEN];
    uint16_t port;
    ip_addr_t ip;
    uint8_t active;
} mdns_service_t;

// mDNS functions
int mdns_init(void);
int mdns_register_service(const char *service_name, uint16_t port);
int mdns_query_service(const char *service_name, ip_addr_t *result_ip, uint16_t *result_port);
void mdns_process_packet(const void *data, uint32_t len, const ip_addr_t *src_ip);
int mdns_send_announcement(const char *service_name, uint16_t port);

#endif // MDNS_H
