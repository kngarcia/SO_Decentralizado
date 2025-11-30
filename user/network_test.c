/* user/network_test.c - User-space network test using syscalls */

// Socket constants
#define AF_INET    2
#define SOCK_DGRAM 2

typedef struct {
    unsigned short sin_family;
    unsigned short sin_port;
    unsigned char sin_addr[4];
    unsigned char sin_zero[8];
} sockaddr_in_t;

// Syscall numbers
#define SYS_LOG      3
#define SYS_SOCKET   15
#define SYS_BIND     16
#define SYS_SENDTO   22
#define SYS_RECVFROM 23

// Syscall wrapper
static inline long syscall(long num, long arg1, long arg2, long arg3) {
    long ret;
    asm volatile (
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(num), "r"(arg1), "r"(arg2), "r"(arg3)
        : "rax", "rdi", "rsi", "rdx"
    );
    return ret;
}

void _start(void) {
    syscall(SYS_LOG, (long)"Network test starting", 0, 0);
    
    // Create UDP socket
    int sock = syscall(SYS_SOCKET, AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        syscall(SYS_LOG, (long)"ERROR: Failed to create socket", 0, 0);
        while(1);
    }
    syscall(SYS_LOG, (long)"Socket created", 0, 0);
    
    // Bind to port 5000
    sockaddr_in_t addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0x8813; // 5000 in network byte order
    addr.sin_addr[0] = 0;
    addr.sin_addr[1] = 0;
    addr.sin_addr[2] = 0;
    addr.sin_addr[3] = 0;
    
    if (syscall(SYS_BIND, sock, (long)&addr, sizeof(addr)) < 0) {
        syscall(SYS_LOG, (long)"ERROR: Failed to bind socket", 0, 0);
        while(1);
    }
    syscall(SYS_LOG, (long)"Socket bound to port 5000", 0, 0);
    
    // Send test message
    const char *msg = "Hello from user space!";
    sockaddr_in_t dest;
    dest.sin_family = AF_INET;
    dest.sin_port = 0x8813; // 5000 in network byte order
    dest.sin_addr[0] = 192;
    dest.sin_addr[1] = 168;
    dest.sin_addr[2] = 1;
    dest.sin_addr[3] = 100;
    
    syscall(SYS_LOG, (long)"Sending test packet", 0, 0);
    long sent = syscall(SYS_SENDTO, sock, (long)msg, 22, (long)&dest);
    if (sent > 0) {
        syscall(SYS_LOG, (long)"Packet sent successfully", 0, 0);
    } else {
        syscall(SYS_LOG, (long)"Failed to send packet", 0, 0);
    }
    
    syscall(SYS_LOG, (long)"Network test complete", 0, 0);
    
    // Exit
    while(1);
}
