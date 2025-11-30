#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "udp.h"
#include "../drivers/serial.h"

static uint16_t ip_id_counter = 1;

static uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

uint16_t ip_checksum(const void *data, uint16_t len) {
    const uint16_t *ptr = (const uint16_t*)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    if (len > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

int ip_init(void) {
    ip_id_counter = 1;
    return 0;
}

int ip_send(netif_t *netif, const ip_addr_t *dest, uint8_t proto, const void *data, uint16_t len) {
    if (!netif || !dest || !data) return -1;
    
    uint8_t packet[IP_HDR_LEN + len];
    ip_hdr_t *hdr = (ip_hdr_t*)packet;
    
    // Build IP header
    hdr->version_ihl = 0x45; // Version 4, IHL 5 (20 bytes)
    hdr->tos = 0;
    hdr->len = htons(IP_HDR_LEN + len);
    hdr->id = htons(ip_id_counter++);
    hdr->frag_off = 0;
    hdr->ttl = 64;
    hdr->proto = proto;
    hdr->checksum = 0;
    ip_addr_copy(&hdr->src, &netif->ip);
    ip_addr_copy(&hdr->dst, dest);
    
    // Calculate checksum
    hdr->checksum = ip_checksum(hdr, IP_HDR_LEN);
    
    // Copy payload
    for (uint16_t i = 0; i < len; i++) {
        packet[IP_HDR_LEN + i] = ((uint8_t*)data)[i];
    }
    
    // Resolve MAC address
    mac_addr_t dest_mac;
    
    // Check if destination is on same subnet
    uint32_t dest_ip = ip_addr_to_u32(dest);
    uint32_t our_ip = ip_addr_to_u32(&netif->ip);
    uint32_t netmask = ip_addr_to_u32(&netif->netmask);
    
    const ip_addr_t *next_hop;
    if ((dest_ip & netmask) == (our_ip & netmask)) {
        next_hop = dest;
    } else {
        next_hop = &netif->gateway;
    }
    
    if (arp_resolve(netif, next_hop, &dest_mac) < 0) {
        // ARP resolution failed, packet will be dropped
        // In real implementation, would queue packet and retry
        return -1;
    }
    
    // Send via Ethernet
    return eth_send(netif, &dest_mac, ETHTYPE_IP, packet, IP_HDR_LEN + len);
}

void ip_input(netif_t *netif, const void *data, uint32_t len) {
    if (!netif || !data || len < IP_HDR_LEN) return;
    
    const ip_hdr_t *hdr = (const ip_hdr_t*)data;
    
    // Verify version
    if ((hdr->version_ihl >> 4) != 4) return;
    
    // Check if packet is for us
    if (ip_addr_cmp(&hdr->dst, &netif->ip) != 0) {
        // Not for us, ignore (no forwarding in this simple implementation)
        return;
    }
    
    // Verify checksum
    uint16_t recv_checksum = hdr->checksum;
    ip_hdr_t hdr_copy = *hdr;
    hdr_copy.checksum = 0;
    uint16_t calc_checksum = ip_checksum(&hdr_copy, IP_HDR_LEN);
    
    if (recv_checksum != calc_checksum) {
        // Checksum mismatch, drop packet
        return;
    }
    
    // Extract payload
    const uint8_t *payload = (const uint8_t*)data + IP_HDR_LEN;
    uint16_t payload_len = htons(hdr->len) - IP_HDR_LEN;
    
    // Dispatch based on protocol
    switch (hdr->proto) {
        case IP_PROTO_UDP:
            udp_input(netif, &hdr->src, payload, payload_len);
            break;
        case IP_PROTO_ICMP:
            // ICMP not implemented yet
            break;
        case IP_PROTO_TCP:
            // TCP not implemented
            break;
        default:
            // Unknown protocol
            break;
    }
}
