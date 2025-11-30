#include "arp.h"
#include "ethernet.h"
#include "../drivers/serial.h"
#include <stddef.h>

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

static uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

int arp_init(void) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = 0;
    }
    return 0;
}

void arp_cache_update(const ip_addr_t *ip, const mac_addr_t *mac) {
    // Check if entry already exists
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ip_addr_cmp(&arp_cache[i].ip, ip) == 0) {
            mac_addr_copy(&arp_cache[i].mac, mac);
            return;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            ip_addr_copy(&arp_cache[i].ip, ip);
            mac_addr_copy(&arp_cache[i].mac, mac);
            arp_cache[i].valid = 1;
            return;
        }
    }
    
    // Cache full, replace first entry (simple FIFO)
    ip_addr_copy(&arp_cache[0].ip, ip);
    mac_addr_copy(&arp_cache[0].mac, mac);
    arp_cache[0].valid = 1;
}

arp_entry_t* arp_cache_lookup(const ip_addr_t *ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ip_addr_cmp(&arp_cache[i].ip, ip) == 0) {
            return &arp_cache[i];
        }
    }
    return NULL;
}

int arp_send_request(netif_t *netif, const ip_addr_t *target_ip) {
    if (!netif || !target_ip) return -1;
    
    arp_packet_t arp;
    
    arp.htype = htons(ARP_HTYPE_ETH);
    arp.ptype = htons(ARP_PTYPE_IP);
    arp.hlen = MAC_ADDR_LEN;
    arp.plen = IP_ADDR_LEN;
    arp.op = htons(ARP_OP_REQUEST);
    
    mac_addr_copy(&arp.sha, &netif->mac);
    ip_addr_copy(&arp.spa, &netif->ip);
    
    // Target MAC is unknown (00:00:00:00:00:00)
    for (int i = 0; i < MAC_ADDR_LEN; i++) {
        arp.tha.addr[i] = 0;
    }
    ip_addr_copy(&arp.tpa, target_ip);
    
    // Send as broadcast
    mac_addr_t broadcast = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    return eth_send(netif, &broadcast, ETHTYPE_ARP, &arp, sizeof(arp));
}

void arp_input(netif_t *netif, const void *data, uint32_t len) {
    if (!netif || !data || len < sizeof(arp_packet_t)) return;
    
    const arp_packet_t *arp = (const arp_packet_t*)data;
    
    uint16_t htype = htons(arp->htype);
    uint16_t ptype = htons(arp->ptype);
    uint16_t op = htons(arp->op);
    
    // Only handle Ethernet/IP
    if (htype != ARP_HTYPE_ETH || ptype != ARP_PTYPE_IP) return;
    
    // Update cache with sender's info
    arp_cache_update(&arp->spa, &arp->sha);
    
    if (op == ARP_OP_REQUEST) {
        // Check if request is for our IP
        if (ip_addr_cmp(&arp->tpa, &netif->ip) == 0) {
            // Send ARP reply
            arp_packet_t reply;
            
            reply.htype = htons(ARP_HTYPE_ETH);
            reply.ptype = htons(ARP_PTYPE_IP);
            reply.hlen = MAC_ADDR_LEN;
            reply.plen = IP_ADDR_LEN;
            reply.op = htons(ARP_OP_REPLY);
            
            mac_addr_copy(&reply.sha, &netif->mac);
            ip_addr_copy(&reply.spa, &netif->ip);
            mac_addr_copy(&reply.tha, &arp->sha);
            ip_addr_copy(&reply.tpa, &arp->spa);
            
            eth_send(netif, &arp->sha, ETHTYPE_ARP, &reply, sizeof(reply));
        }
    } else if (op == ARP_OP_REPLY) {
        // Already updated cache above
    }
}

int arp_resolve(netif_t *netif, const ip_addr_t *ip, mac_addr_t *mac) {
    if (!netif || !ip || !mac) return -1;
    
    // Check cache
    arp_entry_t *entry = arp_cache_lookup(ip);
    if (entry) {
        mac_addr_copy(mac, &entry->mac);
        return 0;
    }
    
    // Send ARP request
    arp_send_request(netif, ip);
    
    // In a real implementation, would wait for reply
    // For now, return error
    return -1;
}
