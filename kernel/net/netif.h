#ifndef NETIF_H
#define NETIF_H

#include <stdint.h>

#define MAC_ADDR_LEN 6
#define IP_ADDR_LEN 4

typedef struct {
    uint8_t addr[MAC_ADDR_LEN];
} mac_addr_t;

typedef struct {
    uint8_t addr[IP_ADDR_LEN];
} ip_addr_t;

typedef struct netif {
    mac_addr_t mac;
    ip_addr_t ip;
    ip_addr_t netmask;
    ip_addr_t gateway;
    void *driver_data;
    
    // Driver functions
    int (*send)(struct netif *netif, const void *data, uint32_t len);
    int (*recv)(struct netif *netif, void *buf, uint32_t max_len);
} netif_t;

// Network interface management
int netif_init(void);
int netif_register(netif_t *netif);
netif_t* netif_get_default(void);
int netif_set_addr(netif_t *netif, ip_addr_t *ip, ip_addr_t *netmask, ip_addr_t *gateway);

// Utility functions
void mac_addr_copy(mac_addr_t *dst, const mac_addr_t *src);
void ip_addr_copy(ip_addr_t *dst, const ip_addr_t *src);
int mac_addr_cmp(const mac_addr_t *a, const mac_addr_t *b);
int ip_addr_cmp(const ip_addr_t *a, const ip_addr_t *b);
uint32_t ip_addr_to_u32(const ip_addr_t *ip);
void u32_to_ip_addr(uint32_t val, ip_addr_t *ip);

#endif // NETIF_H
