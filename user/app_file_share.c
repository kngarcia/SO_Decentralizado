/* user/app_file_share.c
 * Application 2: P2P File Sharing - Distributed file transfer
 * 
 * Features:
 * - Share files with peers over network
 * - Chunk-based transfer protocol
 * - Simple file index and discovery
 */

#include <stdint.h>

#define SYS_EXIT       1
#define SYS_LOG        3
#define SYS_OPEN       10
#define SYS_READ       8
#define SYS_SOCKET     15
#define SYS_BIND       16
#define SYS_SENDTO     22
#define SYS_RECVFROM   23

#define SOCK_DGRAM     2
#define AF_INET        2

typedef struct {
    uint8_t addr[4];
} ip_addr_t;

typedef struct {
    uint16_t family;
    uint16_t port;
    ip_addr_t addr;
} sockaddr_t;

/* File share protocol */
#define CMD_ANNOUNCE   1  /* Announce available file */
#define CMD_REQUEST    2  /* Request file chunk */
#define CMD_DATA       3  /* File data chunk */
#define CMD_LIST       4  /* List available files */

#define CHUNK_SIZE     512

typedef struct {
    uint8_t cmd;
    uint8_t file_id;
    uint16_t chunk_num;
    uint16_t total_chunks;
    uint16_t data_len;
    uint8_t data[CHUNK_SIZE];
} file_packet_t;

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

static void print(const char *msg) {
    syscall(SYS_LOG, (long)msg, 0, 0, 0, 0);
}

void _start(void) {
    print("=== P2P File Share Application ===\n");
    print("Starting distributed file sharing service...\n");
    
    /* Create UDP socket */
    int sockfd = syscall(SYS_SOCKET, AF_INET, SOCK_DGRAM, 0, 0, 0);
    if (sockfd < 0) {
        print("ERROR: Failed to create socket\n");
        syscall(SYS_EXIT, 1, 0, 0, 0, 0);
    }
    
    print("Socket created\n");
    
    /* Bind to port 9999 (file share port) */
    sockaddr_t addr;
    addr.family = AF_INET;
    addr.port = 9999;
    addr.addr.addr[0] = 0;
    addr.addr.addr[1] = 0;
    addr.addr.addr[2] = 0;
    addr.addr.addr[3] = 0;
    
    if (syscall(SYS_BIND, sockfd, (long)&addr, sizeof(addr), 0, 0) < 0) {
        print("ERROR: Failed to bind socket\n");
        syscall(SYS_EXIT, 1, 0, 0, 0, 0);
    }
    
    print("Bound to port 9999\n");
    
    /* Simulate file index */
    const char *files[] = {
        "README.txt",
        "kernel.elf",
        "data.bin"
    };
    const int num_files = 3;
    
    print("File index:\n");
    for (int i = 0; i < num_files; i++) {
        print("  [");
        char id[4] = {'0' + i, ']', ' ', 0};
        print(id);
        print(files[i]);
        print("\n");
    }
    
    /* Announce available files via broadcast */
    print("Announcing available files...\n");
    
    file_packet_t announce_pkt;
    announce_pkt.cmd = CMD_ANNOUNCE;
    announce_pkt.file_id = 0;
    announce_pkt.chunk_num = 0;
    announce_pkt.total_chunks = num_files;
    announce_pkt.data_len = 0;
    
    ip_addr_t broadcast;
    broadcast.addr[0] = 255;
    broadcast.addr[1] = 255;
    broadcast.addr[2] = 255;
    broadcast.addr[3] = 255;
    
    sockaddr_t dest_addr;
    dest_addr.family = AF_INET;
    dest_addr.port = 9999;
    dest_addr.addr = broadcast;
    
    long sent = syscall(SYS_SENDTO, sockfd, (long)&announce_pkt, 
                       sizeof(announce_pkt), 0, (long)&dest_addr);
    
    if (sent > 0) {
        print("Announcement sent\n");
    }
    
    /* Listen for requests */
    print("Listening for file requests...\n");
    
    file_packet_t recv_pkt;
    sockaddr_t src_addr;
    uint32_t addrlen = sizeof(src_addr);
    
    for (int i = 0; i < 10; i++) {
        long received = syscall(SYS_RECVFROM, sockfd, (long)&recv_pkt, 
                               sizeof(recv_pkt), 0, (long)&src_addr);
        
        if (received > 0) {
            print("Received packet: cmd=");
            char cmd[4] = {'0' + recv_pkt.cmd, '\n', 0, 0};
            print(cmd);
            
            if (recv_pkt.cmd == CMD_REQUEST) {
                print("File request received, sending data...\n");
                
                /* Send dummy file chunk */
                file_packet_t data_pkt;
                data_pkt.cmd = CMD_DATA;
                data_pkt.file_id = recv_pkt.file_id;
                data_pkt.chunk_num = 0;
                data_pkt.total_chunks = 1;
                data_pkt.data_len = 32;
                
                /* Fill with test data */
                for (int j = 0; j < 32; j++) {
                    data_pkt.data[j] = 'A' + (j % 26);
                }
                
                dest_addr.addr = src_addr.addr;
                syscall(SYS_SENDTO, sockfd, (long)&data_pkt, 
                       sizeof(data_pkt), 0, (long)&dest_addr);
                
                print("Data chunk sent\n");
            } else if (recv_pkt.cmd == CMD_LIST) {
                print("List request received\n");
            }
        }
        
        /* Delay */
        for (volatile int j = 0; j < 10000000; j++);
    }
    
    print("File share service shutting down\n");
    syscall(SYS_EXIT, 0, 0, 0, 0, 0);
    
    for (;;);
}
