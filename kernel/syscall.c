/* kernel/syscall.c
 * Syscall implementation (int 0x80 / syscall instruction handler)
 * Bridges ring-3 (user-mode) to ring-0 (kernel-mode) services
 */

#include "syscall.h"
#include "process_manager.h"
#include "drivers/serial.h"
#include "elf_loader.h"
#include <string.h>

/* Embedded builtin binaries are referenced via extern to avoid multiple
    definition when headers get included in multiple compilation units. */
extern unsigned char build_user_hello_elf[];
extern unsigned int build_user_hello_elf_len;
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
        process_t *proc = elf_load(build_user_hello_elf, build_user_hello_elf_len);
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
    serial_puts("[sys_close] closed fd="); serial_put_hex(fd); serial_putc('\n');
    return 0;
}
