#include "mdns.h"
#include "udp.h"
#include "netif.h"
#include "../drivers/serial.h"
#include <stddef.h>

static mdns_service_t services[MAX_SERVICES];
static int mdns_socket = -1;

static uint16_t htons(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

// Simple string length
static int str_len(const char *s) {
    int len = 0;
    while (s && s[len]) len++;
    return len;
}

// Simple string copy
static void str_copy(char *dst, const char *src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int mdns_init(void) {
    // Initialize service table
    for (int i = 0; i < MAX_SERVICES; i++) {
        services[i].active = 0;
    }
    
    // Create UDP socket for mDNS
    mdns_socket = udp_socket();
    if (mdns_socket < 0) {
        serial_puts("[mdns] ERROR: Failed to create socket\n");
        return -1;
    }
    
    // Bind to mDNS port
    if (udp_bind(mdns_socket, MDNS_PORT) < 0) {
        serial_puts("[mdns] ERROR: Failed to bind to port 5353\n");
        return -1;
    }
    
    serial_puts("[mdns] Initialized on port 5353\n");
    return 0;
}

int mdns_register_service(const char *service_name, uint16_t port) {
    if (!service_name) return -1;
    
    // Find empty slot
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (!services[i].active) {
            str_copy(services[i].service_name, service_name, MAX_SERVICE_NAME_LEN);
            services[i].port = port;
            
            // Get our IP address
            netif_t *netif = netif_get_default();
            if (netif) {
                ip_addr_copy(&services[i].ip, &netif->ip);
            }
            
            services[i].active = 1;
            
            serial_puts("[mdns] Registered service: ");
            serial_puts(service_name);
            serial_puts(" on port ");
            serial_put_hex(port);
            serial_putc('\n');
            
            // Send announcement
            mdns_send_announcement(service_name, port);
            
            return 0;
        }
    }
    
    return -1; // No free slots
}

int mdns_send_announcement(const char *service_name, uint16_t port) {
    if (!service_name) return -1;
    
    // Build simple mDNS response packet
    uint8_t packet[512];
    int offset = 0;
    
    dns_header_t *hdr = (dns_header_t *)packet;
    hdr->id = 0;
    hdr->flags = htons(DNS_FLAG_RESPONSE | DNS_FLAG_AUTHORITATIVE);
    hdr->qdcount = 0;
    hdr->ancount = htons(1); // One answer
    hdr->nscount = 0;
    hdr->arcount = 0;
    offset += sizeof(dns_header_t);
    
    // Encode service name as DNS label
    int name_len = str_len(service_name);
    if (name_len > 63) name_len = 63;
    packet[offset++] = name_len;
    for (int i = 0; i < name_len; i++) {
        packet[offset++] = service_name[i];
    }
    
    // Add ".local" suffix
    packet[offset++] = 5;
    packet[offset++] = 'l';
    packet[offset++] = 'o';
    packet[offset++] = 'c';
    packet[offset++] = 'a';
    packet[offset++] = 'l';
    packet[offset++] = 0; // End of name
    
    // Type (A record)
    packet[offset++] = 0;
    packet[offset++] = DNS_TYPE_A;
    
    // Class (IN - Internet, with cache-flush bit)
    packet[offset++] = 0x80;
    packet[offset++] = 0x01;
    
    // TTL
    uint32_t ttl = MDNS_TTL;
    packet[offset++] = (ttl >> 24) & 0xFF;
    packet[offset++] = (ttl >> 16) & 0xFF;
    packet[offset++] = (ttl >> 8) & 0xFF;
    packet[offset++] = ttl & 0xFF;
    
    // Data length (4 bytes for IPv4)
    packet[offset++] = 0;
    packet[offset++] = 4;
    
    // IP address
    netif_t *netif = netif_get_default();
    if (netif) {
        for (int i = 0; i < 4; i++) {
            packet[offset++] = netif->ip.addr[i];
        }
    } else {
        offset += 4;
    }
    
    // Send to mDNS multicast address
    ip_addr_t mdns_ip = {{MDNS_MCAST_IP_0, MDNS_MCAST_IP_1, MDNS_MCAST_IP_2, MDNS_MCAST_IP_3}};
    return udp_sendto(mdns_socket, packet, offset, &mdns_ip, MDNS_PORT);
}

int mdns_query_service(const char *service_name, ip_addr_t *result_ip, uint16_t *result_port) {
    if (!service_name) return -1;
    
    // Check local cache first
    for (int i = 0; i < MAX_SERVICES; i++) {
        if (services[i].active) {
            // Simple string comparison
            int match = 1;
            for (int j = 0; j < MAX_SERVICE_NAME_LEN; j++) {
                if (services[i].service_name[j] != service_name[j]) {
                    match = 0;
                    break;
                }
                if (service_name[j] == '\0') break;
            }
            
            if (match) {
                if (result_ip) ip_addr_copy(result_ip, &services[i].ip);
                if (result_port) *result_port = services[i].port;
                return 0;
            }
        }
    }
    
    // Would send query packet here in full implementation
    serial_puts("[mdns] Service not found in cache: ");
    serial_puts(service_name);
    serial_putc('\n');
    
    return -1;
}

void mdns_process_packet(const void *data, uint32_t len, const ip_addr_t *src_ip) {
    if (!data || len < sizeof(dns_header_t)) return;
    
    const dns_header_t *hdr = (const dns_header_t *)data;
    
    // Check if it's a query or response
    uint16_t flags = htons(hdr->flags);
    
    if (flags & DNS_FLAG_RESPONSE) {
        // Process response (cache peer services)
        serial_puts("[mdns] Received mDNS response from peer\n");
        // Would parse and cache service info here
    } else {
        // Process query (respond if we have the service)
        serial_puts("[mdns] Received mDNS query\n");
        // Would parse query and send response if we have the service
    }
}
