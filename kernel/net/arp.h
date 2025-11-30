#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include "netif.h"

#define ARP_HTYPE_ETH  1
#define ARP_PTYPE_IP   0x0800
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

#define ARP_CACHE_SIZE 16

typedef struct {
    uint16_t htype;      // Hardware type
    uint16_t ptype;      // Protocol type
    uint8_t hlen;        // Hardware address length
    uint8_t plen;        // Protocol address length
    uint16_t op;         // Operation
    mac_addr_t sha;      // Sender hardware address
    ip_addr_t spa;       // Sender protocol address
    mac_addr_t tha;      // Target hardware address
    ip_addr_t tpa;       // Target protocol address
} __attribute__((packed)) arp_packet_t;

typedef struct {
    ip_addr_t ip;
    mac_addr_t mac;
    uint8_t valid;
} arp_entry_t;

// ARP functions
int arp_init(void);
int arp_resolve(netif_t *netif, const ip_addr_t *ip, mac_addr_t *mac);
void arp_input(netif_t *netif, const void *data, uint32_t len);
int arp_send_request(netif_t *netif, const ip_addr_t *target_ip);
void arp_cache_update(const ip_addr_t *ip, const mac_addr_t *mac);
arp_entry_t* arp_cache_lookup(const ip_addr_t *ip);

#endif // ARP_H
