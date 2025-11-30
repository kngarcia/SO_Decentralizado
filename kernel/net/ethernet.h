#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include "netif.h"

#define ETHTYPE_IP   0x0800
#define ETHTYPE_ARP  0x0806
#define ETH_HLEN     14
#define ETH_FRAME_LEN 1514

typedef struct {
    mac_addr_t dest;
    mac_addr_t src;
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

typedef struct {
    eth_hdr_t hdr;
    uint8_t payload[ETH_FRAME_LEN - ETH_HLEN];
} __attribute__((packed)) eth_frame_t;

// Ethernet layer functions
int eth_init(void);
int eth_send(netif_t *netif, const mac_addr_t *dest, uint16_t type, const void *data, uint16_t len);
int eth_recv(netif_t *netif, eth_frame_t *frame);
void eth_input(netif_t *netif, const void *data, uint32_t len);

#endif // ETHERNET_H
