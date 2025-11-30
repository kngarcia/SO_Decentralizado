/* kernel/syscall.c
 * Syscall implementation (int 0x80 / syscall instruction handler)
 * Bridges ring-3 (user-mode) to ring-0 (kernel-mode) services
 */

#include "syscall.h"
#include "process_manager.h"
#include "drivers/serial.h"
#include "elf_loader.h"
#include "net/udp.h"
#include "net/netif.h"
#include <string.h>

/* Embedded builtin binaries are referenced via extern to avoid multiple
    definition when headers get included in multiple compilation units. */
extern unsigned char build_user_hello_nocet_elf[];
extern unsigned int build_user_hello_nocet_elf_len;
#include <stddef.h>

/* Syscall handler entry point
 * Receives syscall number + 3 args in RDI, RSI, RDX (x86-64 calling convention)
 * Returns result in RAX
 */
void syscall_install(void) {
    /* TODO: Setup int 0x80 handler (legacy x86 trap)
     * TODO: Setup syscall/sysret MSR (fast path on x86-64)
     */
    serial_puts("[syscall] installed int 0x80 handler\n");
}

/* Dispatcher - routes syscall number to handler */
syscall_result_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    /* Debug: log incoming syscall numbers for tracing */
    serial_puts("[syscall] num=");
    serial_put_hex(num);
    serial_puts(" arg1=0x");
    serial_put_hex(arg1);
    serial_putc('\n');
    switch (num) {
        case SYS_EXIT:
            sys_exit((int)arg1);
            return 0;
        case SYS_YIELD:
            sys_yield();
            return 0;
        case SYS_LOG:
            return sys_log((const char *)arg1);
        case SYS_MMAP:
            return (int64_t)sys_mmap(arg1, arg2, (int)arg3);
        case SYS_FORK:
            return sys_fork();
        case SYS_EXEC:
            return sys_exec((const char *)arg1, (char **)arg2);
        case SYS_WAIT:
            return sys_wait((int)arg1);
        case SYS_READ:
            return sys_read((int)arg1, (void *)arg2, (int)arg3);
        case SYS_WRITE:
            return sys_write((int)arg1, (const void *)arg2, (int)arg3);
        case SYS_OPEN:
            return sys_open((const char *)arg1, (int)arg2);
        case SYS_CLOSE:
            return sys_close((int)arg1);
        case SYS_WASM_LOAD:
            return sys_wasm_load((const void *)arg1, arg2, (const char *)arg3);
        case SYS_WASM_EXEC:
            return sys_wasm_exec((int)arg1, (const char *)arg2);
        case SYS_SOCKET:
            return sys_socket((int)arg1, (int)arg2, (int)arg3);
        case SYS_BIND:
            return sys_bind((int)arg1, (const void *)arg2, (uint32_t)arg3);
        case SYS_SENDTO:
            return sys_sendto((int)arg1, (const void *)arg2, (uint32_t)arg3, 0, NULL, 0);
        case SYS_RECVFROM:
            return sys_recvfrom((int)arg1, (void *)arg2, (uint32_t)arg3, 0, NULL, NULL);
        default:
            return -1; /* EINVAL */
    }
}

/* ============================================================
 * Individual Syscall Implementations (Stubs for now)
 * ============================================================
 */

void sys_exit(int code) {
    serial_puts("[sys_exit] called\n");
    extern process_t *pm_get_current(void);
    process_t *cur = pm_get_current();
    if (cur) {
        /* Close all file descriptors held by process */
        extern void fs_decref(int fd);
        for (int i = 0; i < 16; ++i) {
            if (cur->fds[i] >= 0) fs_decref(cur->fds[i]);
        }
        cur->exit_code = code;
        cur->state = 3; /* dead */
    }
    /* For now busy-yield to let scheduler pick next task */
    while (1) sys_yield();
}

void sys_yield(void) {
    /* Yield CPU to next task (triggers round-robin)
     * TODO: Call scheduler_yield()
     */
    serial_puts("[sys_yield] called\n");
}

syscall_result_t sys_log(const char *msg) {
    /* Write message to serial port (kernel logging) */
    if (!msg) return -1;
    serial_puts("[user] ");
    serial_puts(msg);
    serial_putc('\n');
    return 0;
}

void *sys_mmap(uint64_t addr, uint64_t size, int prot) {
    /* Allocate virtual memory region
     * Prot flags: PROT_READ=0x1, PROT_WRITE=0x2, PROT_EXEC=0x4
     * TODO: Call virtual_memory_alloc(addr, size, prot)
     */
    serial_puts("[sys_mmap] called\n");
    return NULL;
}

int sys_fork(void) {
    /* Minimal prototype for fork: allocate a new process slot using
     * process_create() and return the child PID. This is a temporary
     * Phase-1 implementation to allow basic multi-process bookkeeping
     * and to be extended later with full cloning of memory/PCB.
     */
    /* Prefer cloning an existing user/kernel process if available via process manager */
    extern process_t *pm_clone_process(process_t *parent);
    extern process_t *pm_get_current(void);

    process_t *cur = pm_get_current();
    if (cur) {
        process_t *child = pm_clone_process(cur);
        if (!child) {
            serial_puts("[sys_fork] pm_clone failed\n");
            return -1;
        }
        /* Ensure child will see 0 as fork return value */
        child->fork_ret = 0;
        return (int)child->pid;
    }

    /* Fallback for kernel tasks: allocate a new process slot */
    extern int process_create(void (*entry)(void));
    int child_pid = process_create(NULL);
    if (child_pid < 0) {
        serial_puts("[sys_fork] failed: no free process slots\n");
        return -1;
    }

    serial_puts("[sys_fork] created pid ");
    serial_put_hex((uint64_t)child_pid);
    serial_putc('\n');

    /* Return child's PID to the caller (parent). A real fork would
     * return 0 in the child context but that requires scheduler support
     * and a full process context switch. We'll keep this simple for Phase 1.
     */
    return child_pid;
}

int sys_exec(const char *path, char **argv) {
    /* Minimal sys_exec for Phase1:
     * - If path is NULL or equals "/hello" or "hello", load embedded user_hello ELF
     * - Call elf_load() then elf_exec() which performs an iretq to ring-3.
     * This is a Phase1 convenience implementation (no filesystem yet).
     */
    if (!path || strcmp(path, "/hello") == 0 || strcmp(path, "hello") == 0) {
        process_t *proc = elf_load(build_user_hello_nocet_elf, build_user_hello_nocet_elf_len);
        if (!proc) {
            serial_puts("[sys_exec] elf_load failed\n");
            return -1;
        }

        /* Execute will not return on success (iretq -> ring-3) */
        return elf_exec(proc, argv, NULL);
    }

    serial_puts("[sys_exec] path not recognized (no FS)\n");
    return -1;
}

int sys_wait(int pid) {
    serial_puts("[sys_wait] waiting for pid= ");
    serial_put_hex((uint64_t)pid);
    serial_putc('\n');
    extern process_t *pm_find_by_pid(uint64_t pid);
    process_t *p;
    while (1) {
        p = pm_find_by_pid((uint64_t)pid);
        if (!p) {
            /* no such pid */
            return -1;
        }
        if (p->state == 3) {
            /* child exited */
            return p->exit_code;
        }
        sys_yield();
    }
}

int sys_read(int fd, void *buf, int count) {
    /* Read from file descriptor
     * TODO: Route to file system or device driver
     */
    serial_puts("[sys_read] called (not implemented)\n");
    return -1;
}

int sys_write(int fd, const void *buf, int count) {
    /* Write to file descriptor
     * fd=1 -> stdout (serial), fd=2 -> stderr (serial)
     */
    if (fd == 1 || fd == 2) {
        const char *p = (const char *)buf;
        for (int i = 0; i < count && p[i]; i++) {
            serial_putc(p[i]);
        }
        return count;
    }
    return -1;
}

int sys_open(const char *path, int flags) {
    /* Phase1: allocate a simple in-kernel file descriptor; path/flags ignored */
    extern int fs_alloc(void);
    int fd = fs_alloc();
    if (fd < 0) return -1;
    serial_puts("[sys_open] allocated fd="); serial_put_hex(fd); serial_putc('\n');
    return fd;
}

int sys_close(int fd) {
    extern void fs_decref(int fd);
    fs_decref(fd);
    serial_puts("[sys_close] fd="); serial_put_hex(fd); serial_putc('\n');
    return 0;
}

/* ============================================================
 * WASM3 Syscall Implementations
 * ============================================================
 */
#include "wasm/wasm_wrapper.h"

int sys_wasm_load(const void *wasm_bytes, uint64_t len, const char *module_name) {
    serial_puts("[sys_wasm_load] Loading WASM module: ");
    serial_puts(module_name ? module_name : "(unnamed)");
    serial_putc('\n');
    
    if (!wasm_bytes || len == 0) {
        serial_puts("[sys_wasm_load] ERROR: Invalid WASM bytes\n");
        return -1;
    }
    
    int module_id = wasm_load_module((const uint8_t *)wasm_bytes, (size_t)len, module_name ? module_name : "module");
    if (module_id < 0) {
        serial_puts("[sys_wasm_load] ERROR: ");
        serial_puts(wasm_get_last_error());
        serial_putc('\n');
        return -1;
    }
    
    serial_puts("[sys_wasm_load] SUCCESS: module_id=");
    serial_put_hex(module_id);
    serial_putc('\n');
    return module_id;
}

int sys_wasm_exec(int module_id, const char *func_name) {
    serial_puts("[sys_wasm_exec] Executing ");
    serial_puts(func_name ? func_name : "(null)");
    serial_puts(" in module ");
    serial_put_hex(module_id);
    serial_putc('\n');
    
    if (!func_name) {
        serial_puts("[sys_wasm_exec] ERROR: func_name is NULL\n");
        return -1;
    }
    
    int result = wasm_exec_function(module_id, func_name, 0, NULL);
    if (result < 0) {
        serial_puts("[sys_wasm_exec] ERROR: ");
        serial_puts(wasm_get_last_error());
        serial_putc('\n');
        return -1;
    }
    
    serial_puts("[sys_wasm_exec] SUCCESS: result=");
    serial_put_hex(result);
    serial_putc('\n');
    return result;
}

/* ============================================================
 * Network Syscall Implementations (Phase 3)
 * ============================================================
 */

#define AF_INET    2
#define SOCK_DGRAM 2

typedef struct {
    uint16_t sin_family;
    uint16_t sin_port;
    uint8_t sin_addr[4];
    uint8_t sin_zero[8];
} sockaddr_in_t;

int sys_socket(int domain, int type, int protocol) {
    serial_puts("[sys_socket] Creating socket\n");
    
    // Only support UDP for now
    if (domain != AF_INET || type != SOCK_DGRAM) {
        serial_puts("[sys_socket] ERROR: Only AF_INET/SOCK_DGRAM supported\n");
        return -1;
    }
    
    return udp_socket();
}

int sys_bind(int sockfd, const void *addr, uint32_t addrlen) {
    serial_puts("[sys_bind] Binding socket ");
    serial_put_hex(sockfd);
    serial_putc('\n');
    
    if (!addr || addrlen < sizeof(sockaddr_in_t)) return -1;
    
    const sockaddr_in_t *sa = (const sockaddr_in_t *)addr;
    
    // Convert network byte order to host byte order
    uint16_t port = ((sa->sin_port & 0xFF) << 8) | ((sa->sin_port >> 8) & 0xFF);
    
    return udp_bind(sockfd, port);
}

int sys_connect(int sockfd, const void *addr, uint32_t addrlen) {
    // UDP is connectionless, just return success
    return 0;
}

int sys_listen(int sockfd, int backlog) {
    // Not applicable for UDP
    return -1;
}

int sys_accept(int sockfd, void *addr, uint32_t *addrlen) {
    // Not applicable for UDP
    return -1;
}

int sys_send(int sockfd, const void *buf, uint32_t len, int flags) {
    // For UDP, need destination address
    return -1;
}

int sys_recv(int sockfd, void *buf, uint32_t len, int flags) {
    // Use recvfrom for UDP
    return sys_recvfrom(sockfd, buf, len, flags, NULL, NULL);
}

int sys_sendto(int sockfd, const void *buf, uint32_t len, int flags, const void *dest_addr, uint32_t addrlen) {
    if (!buf) return -1;
    
    // For simplified implementation, use hardcoded destination for now
    // In real implementation, would parse dest_addr
    netif_t *netif = netif_get_default();
    if (!netif) return -1;
    
    ip_addr_t dest_ip = {{192, 168, 1, 100}}; // Placeholder
    uint16_t dest_port = 5000;
    
    if (dest_addr && addrlen >= sizeof(sockaddr_in_t)) {
        const sockaddr_in_t *sa = (const sockaddr_in_t *)dest_addr;
        for (int i = 0; i < 4; i++) {
            dest_ip.addr[i] = sa->sin_addr[i];
        }
        dest_port = ((sa->sin_port & 0xFF) << 8) | ((sa->sin_port >> 8) & 0xFF);
    }
    
    return udp_sendto(sockfd, buf, (uint16_t)len, &dest_ip, dest_port);
}

int sys_recvfrom(int sockfd, void *buf, uint32_t len, int flags, void *src_addr, uint32_t *addrlen) {
    if (!buf) return -1;
    
    ip_addr_t src_ip;
    uint16_t src_port;
    
    int result = udp_recvfrom(sockfd, buf, (uint16_t)len, &src_ip, &src_port);
    
    if (result > 0 && src_addr && addrlen && *addrlen >= sizeof(sockaddr_in_t)) {
        sockaddr_in_t *sa = (sockaddr_in_t *)src_addr;
        sa->sin_family = AF_INET;
        sa->sin_port = ((src_port & 0xFF) << 8) | ((src_port >> 8) & 0xFF);
        for (int i = 0; i < 4; i++) {
            sa->sin_addr[i] = src_ip.addr[i];
        }
        *addrlen = sizeof(sockaddr_in_t);
    }
    
    return result;
}
