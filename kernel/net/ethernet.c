#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "../drivers/serial.h"

static uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

int eth_init(void) {
    return 0;
}

int eth_send(netif_t *netif, const mac_addr_t *dest, uint16_t type, const void *data, uint16_t len) {
    if (!netif || !dest || !data || len == 0) return -1;
    if (len > ETH_FRAME_LEN - ETH_HLEN) return -1;
    
    eth_frame_t frame;
    
    // Build Ethernet header
    mac_addr_copy(&frame.hdr.dest, dest);
    mac_addr_copy(&frame.hdr.src, &netif->mac);
    frame.hdr.type = htons(type);
    
    // Copy payload
    for (uint16_t i = 0; i < len; i++) {
        frame.payload[i] = ((uint8_t*)data)[i];
    }
    
    // Send frame
    return netif->send(netif, &frame, ETH_HLEN + len);
}

int eth_recv(netif_t *netif, eth_frame_t *frame) {
    if (!netif || !frame) return -1;
    
    return netif->recv(netif, frame, sizeof(eth_frame_t));
}

void eth_input(netif_t *netif, const void *data, uint32_t len) {
    if (!netif || !data || len < ETH_HLEN) return;
    
    const eth_hdr_t *hdr = (const eth_hdr_t*)data;
    const uint8_t *payload = (const uint8_t*)data + ETH_HLEN;
    uint16_t payload_len = len - ETH_HLEN;
    uint16_t ethtype = htons(hdr->type);
    
    switch (ethtype) {
        case ETHTYPE_ARP:
            arp_input(netif, payload, payload_len);
            break;
        case ETHTYPE_IP:
            ip_input(netif, payload, payload_len);
            break;
        default:
            // Unknown protocol, ignore
            break;
    }
}
