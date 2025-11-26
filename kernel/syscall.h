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

#endif /* SYSCALL_H */
