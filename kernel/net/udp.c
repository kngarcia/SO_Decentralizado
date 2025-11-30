#include "udp.h"
#include "ip.h"
#include "netif.h"
#include "../drivers/serial.h"

static udp_socket_t udp_sockets[MAX_UDP_SOCKETS];

static uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

int udp_init(void) {
    for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
        udp_sockets[i].in_use = 0;
        udp_sockets[i].rx_ready = 0;
    }
    return 0;
}

int udp_socket(void) {
    for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
        if (!udp_sockets[i].in_use) {
            udp_sockets[i].in_use = 1;
            udp_sockets[i].local_port = 0;
            udp_sockets[i].rx_ready = 0;
            return i;
        }
    }
    return -1; // No free sockets
}

int udp_bind(int sockfd, uint16_t port) {
    if (sockfd < 0 || sockfd >= MAX_UDP_SOCKETS) return -1;
    if (!udp_sockets[sockfd].in_use) return -1;
    
    // Check if port is already in use
    for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
        if (udp_sockets[i].in_use && udp_sockets[i].local_port == port) {
            return -1;
        }
    }
    
    udp_sockets[sockfd].local_port = port;
    return 0;
}

int udp_sendto(int sockfd, const void *data, uint16_t len, const ip_addr_t *dest_ip, uint16_t dest_port) {
    if (sockfd < 0 || sockfd >= MAX_UDP_SOCKETS) return -1;
    if (!udp_sockets[sockfd].in_use) return -1;
    if (!data || len == 0) return -1;
    
    netif_t *netif = netif_get_default();
    if (!netif) return -1;
    
    uint8_t packet[UDP_HDR_LEN + len];
    udp_hdr_t *hdr = (udp_hdr_t*)packet;
    
    // Build UDP header
    hdr->src_port = htons(udp_sockets[sockfd].local_port);
    hdr->dst_port = htons(dest_port);
    hdr->len = htons(UDP_HDR_LEN + len);
    hdr->checksum = 0; // Optional in IPv4
    
    // Copy payload
    for (uint16_t i = 0; i < len; i++) {
        packet[UDP_HDR_LEN + i] = ((uint8_t*)data)[i];
    }
    
    // Send via IP layer
    return ip_send(netif, dest_ip, IP_PROTO_UDP, packet, UDP_HDR_LEN + len);
}

int udp_recvfrom(int sockfd, void *buf, uint16_t max_len, ip_addr_t *src_ip, uint16_t *src_port) {
    if (sockfd < 0 || sockfd >= MAX_UDP_SOCKETS) return -1;
    if (!udp_sockets[sockfd].in_use) return -1;
    if (!buf) return -1;
    
    udp_socket_t *sock = &udp_sockets[sockfd];
    
    // Check if data is available
    if (!sock->rx_ready) return 0;
    
    // Copy data
    uint16_t copy_len = sock->rx_len;
    if (copy_len > max_len) copy_len = max_len;
    
    for (uint16_t i = 0; i < copy_len; i++) {
        ((uint8_t*)buf)[i] = sock->rx_buf[i];
    }
    
    // Copy source info
    if (src_ip) ip_addr_copy(src_ip, &sock->rx_src_ip);
    if (src_port) *src_port = sock->rx_src_port;
    
    // Clear receive flag
    sock->rx_ready = 0;
    
    return copy_len;
}

void udp_input(netif_t *netif, const ip_addr_t *src_ip, const void *data, uint32_t len) {
    if (!netif || !src_ip || !data || len < UDP_HDR_LEN) return;
    
    const udp_hdr_t *hdr = (const udp_hdr_t*)data;
    const uint8_t *payload = (const uint8_t*)data + UDP_HDR_LEN;
    uint16_t payload_len = len - UDP_HDR_LEN;
    
    uint16_t dst_port = htons(hdr->dst_port);
    uint16_t src_port = htons(hdr->src_port);
    
    // Find socket bound to destination port
    for (int i = 0; i < MAX_UDP_SOCKETS; i++) {
        if (udp_sockets[i].in_use && udp_sockets[i].local_port == dst_port) {
            // Deliver to this socket
            udp_socket_t *sock = &udp_sockets[i];
            
            // Copy to receive buffer (overwrite if previous packet not read)
            uint16_t copy_len = payload_len;
            if (copy_len > sizeof(sock->rx_buf)) copy_len = sizeof(sock->rx_buf);
            
            for (uint16_t j = 0; j < copy_len; j++) {
                sock->rx_buf[j] = payload[j];
            }
            
            sock->rx_len = copy_len;
            ip_addr_copy(&sock->rx_src_ip, src_ip);
            sock->rx_src_port = src_port;
            sock->rx_ready = 1;
            
            return;
        }
    }
    
    // No socket found, drop packet
}

int udp_close(int sockfd) {
    if (sockfd < 0 || sockfd >= MAX_UDP_SOCKETS) return -1;
    if (!udp_sockets[sockfd].in_use) return -1;
    
    udp_sockets[sockfd].in_use = 0;
    return 0;
}
