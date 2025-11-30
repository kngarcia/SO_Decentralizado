#include "netif.h"
#include <stddef.h>

static netif_t *default_netif = NULL;

int netif_init(void) {
    default_netif = NULL;
    return 0;
}

int netif_register(netif_t *netif) {
    if (!netif) return -1;
    if (!default_netif) {
        default_netif = netif;
    }
    return 0;
}

netif_t* netif_get_default(void) {
    return default_netif;
}

int netif_set_addr(netif_t *netif, ip_addr_t *ip, ip_addr_t *netmask, ip_addr_t *gateway) {
    if (!netif) return -1;
    if (ip) ip_addr_copy(&netif->ip, ip);
    if (netmask) ip_addr_copy(&netif->netmask, netmask);
    if (gateway) ip_addr_copy(&netif->gateway, gateway);
    return 0;
}

void mac_addr_copy(mac_addr_t *dst, const mac_addr_t *src) {
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        dst->addr[i] = src->addr[i];
    }
}

void ip_addr_copy(ip_addr_t *dst, const ip_addr_t *src) {
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        dst->addr[i] = src->addr[i];
    }
}

int mac_addr_cmp(const mac_addr_t *a, const mac_addr_t *b) {
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        if (a->addr[i] != b->addr[i]) return 1;
    }
    return 0;
}

int ip_addr_cmp(const ip_addr_t *a, const ip_addr_t *b) {
    for (int i = 0; i < IP_ADDR_LEN; i++) {
        if (a->addr[i] != b->addr[i]) return 1;
    }
    return 0;
}

uint32_t ip_addr_to_u32(const ip_addr_t *ip) {
    return ((uint32_t)ip->addr[0] << 24) |
           ((uint32_t)ip->addr[1] << 16) |
           ((uint32_t)ip->addr[2] << 8) |
           ((uint32_t)ip->addr[3]);
}

void u32_to_ip_addr(uint32_t val, ip_addr_t *ip) {
    ip->addr[0] = (val >> 24) & 0xFF;
    ip->addr[1] = (val >> 16) & 0xFF;
    ip->addr[2] = (val >> 8) & 0xFF;
    ip->addr[3] = val & 0xFF;
}
