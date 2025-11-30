/* kernel/syscall.h
 * Syscall interface (int 0x80 - x86 legacy + syscall instruction for x86-64)
 * Allows ring-3 (user-mode) applications to request kernel services
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* Syscall numbers (ARM/x86 cross-compatible convention) */
#define SYS_EXIT       1
#define SYS_YIELD      2
#define SYS_LOG        3
#define SYS_MMAP       4
#define SYS_FORK       5
#define SYS_EXEC       6
#define SYS_WAIT       7
#define SYS_READ       8
#define SYS_WRITE      9
#define SYS_OPEN       10
#define SYS_CLOSE      11
#define SYS_STAT       12
#define SYS_WASM_LOAD  13
#define SYS_WASM_EXEC  14

/* Network syscalls (Phase 3) */
#define SYS_SOCKET     15
#define SYS_BIND       16
#define SYS_CONNECT    17
#define SYS_LISTEN     18
#define SYS_ACCEPT     19
#define SYS_SEND       20
#define SYS_RECV       21
#define SYS_SENDTO     22
#define SYS_RECVFROM   23

/* Syscall return type */
typedef int64_t syscall_result_t;

/* Register syscall handler (called from start.S trap handler) */
void syscall_install(void);

/* Syscall dispatcher (entry point from interrupt handler) */
syscall_result_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

/* Individual syscall implementations */
void sys_exit(int code);
void sys_yield(void);
syscall_result_t sys_log(const char *msg);
void *sys_mmap(uint64_t addr, uint64_t size, int prot);
int sys_fork(void);
int sys_exec(const char *path, char **argv);
int sys_wait(int pid);
int sys_read(int fd, void *buf, int count);
int sys_write(int fd, const void *buf, int count);
int sys_open(const char *path, int flags);
int sys_close(int fd);
int sys_wasm_load(const void *wasm_bytes, uint64_t len, const char *module_name);
int sys_wasm_exec(int module_id, const char *func_name);

/* Network syscalls (Phase 3) */
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, const void *addr, uint32_t addrlen);
int sys_connect(int sockfd, const void *addr, uint32_t addrlen);
int sys_listen(int sockfd, int backlog);
int sys_accept(int sockfd, void *addr, uint32_t *addrlen);
int sys_send(int sockfd, const void *buf, uint32_t len, int flags);
int sys_recv(int sockfd, void *buf, uint32_t len, int flags);
int sys_sendto(int sockfd, const void *buf, uint32_t len, int flags, const void *dest_addr, uint32_t addrlen);
int sys_recvfrom(int sockfd, void *buf, uint32_t len, int flags, void *src_addr, uint32_t *addrlen);

#endif /* SYSCALL_H */
