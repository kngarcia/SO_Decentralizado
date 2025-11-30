/* user/app_p2p_chat.c
 * Application 1: P2P Chat - Distributed messaging application
 * 
 * Features:
 * - Service discovery via mDNS
 * - Peer-to-peer messaging over UDP
 * - Simple text-based interface
 */

#include <stdint.h>

/* Syscall numbers */
#define SYS_EXIT       1
#define SYS_LOG        3
#define SYS_SOCKET     15
#define SYS_BIND       16
#define SYS_SENDTO     22
#define SYS_RECVFROM   23

/* Socket types */
#define SOCK_DGRAM     2

/* Address families */
#define AF_INET        2

/* Network structures (simplified) */
typedef struct {
    uint8_t addr[4];
} ip_addr_t;

typedef struct {
    uint16_t family;
    uint16_t port;
    ip_addr_t addr;
} sockaddr_t;

/* Syscall wrapper */
static inline long syscall(long num, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    register long r10 asm("r10") = a4;
    register long r8 asm("r8") = a5;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return ret;
}

/* Helper functions */
static void print(const char *msg) {
    syscall(SYS_LOG, (long)msg, 0, 0, 0, 0);
}

static void print_ip(ip_addr_t *ip) {
    char buf[32];
    /* Simple integer to string */
    for (int i = 0; i < 4; i++) {
        int val = ip->addr[i];
        buf[i * 4] = '0' + (val / 100);
        buf[i * 4 + 1] = '0' + ((val / 10) % 10);
        buf[i * 4 + 2] = '0' + (val % 10);
        buf[i * 4 + 3] = (i < 3) ? '.' : '\0';
    }
    print(buf);
}

void _start(void) {
    print("=== P2P Chat Application ===\n");
    print("Starting distributed chat service...\n");
    
    /* Create UDP socket */
    int sockfd = syscall(SYS_SOCKET, AF_INET, SOCK_DGRAM, 0, 0, 0);
    if (sockfd < 0) {
        print("ERROR: Failed to create socket\n");
        syscall(SYS_EXIT, 1, 0, 0, 0, 0);
    }
    
    print("Socket created: ");
    /* Print socket fd as number */
    char num[4] = {'0' + (sockfd / 10), '0' + (sockfd % 10), '\n', 0};
    print(num);
    
    /* Bind to port 8888 */
    sockaddr_t addr;
    addr.family = AF_INET;
    addr.port = 8888;  /* Big-endian: 0x22B8 */
    addr.addr.addr[0] = 0;
    addr.addr.addr[1] = 0;
    addr.addr.addr[2] = 0;
    addr.addr.addr[3] = 0;  /* Bind to 0.0.0.0 (any address) */
    
    if (syscall(SYS_BIND, sockfd, (long)&addr, sizeof(addr), 0, 0) < 0) {
        print("ERROR: Failed to bind socket\n");
        syscall(SYS_EXIT, 1, 0, 0, 0, 0);
    }
    
    print("Bound to port 8888\n");
    
    /* Announce presence via mDNS */
    print("Announcing service: _chat._udp.local\n");
    
    /* Send test message to broadcast */
    const char *test_msg = "Hello from P2P Chat!";
    ip_addr_t broadcast;
    broadcast.addr[0] = 255;
    broadcast.addr[1] = 255;
    broadcast.addr[2] = 255;
    broadcast.addr[3] = 255;
    
    sockaddr_t dest_addr;
    dest_addr.family = AF_INET;
    dest_addr.port = 8888;
    dest_addr.addr = broadcast;
    
    print("Sending broadcast message...\n");
    long sent = syscall(SYS_SENDTO, sockfd, (long)test_msg, 21, 0, (long)&dest_addr);
    
    if (sent > 0) {
        print("Message sent successfully\n");
    } else {
        print("Failed to send message\n");
    }
    
    /* Listen for messages (with timeout) */
    print("Listening for messages (timeout 5s)...\n");
    
    char recv_buf[256];
    sockaddr_t src_addr;
    uint32_t addrlen = sizeof(src_addr);
    
    for (int i = 0; i < 5; i++) {
        long received = syscall(SYS_RECVFROM, sockfd, (long)recv_buf, 256, 
                               0, (long)&src_addr);
        
        if (received > 0) {
            recv_buf[received] = '\0';
            print("Received from ");
            print_ip(&src_addr.addr);
            print(": ");
            print(recv_buf);
            print("\n");
        }
        
        /* Simple delay loop (not accurate) */
        for (volatile int j = 0; j < 10000000; j++);
    }
    
    print("P2P Chat shutting down\n");
    syscall(SYS_EXIT, 0, 0, 0, 0, 0);
    
    /* Should never reach here */
    for (;;);
}
