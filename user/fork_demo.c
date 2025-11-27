#include <stdint.h>

/* Simple user-mode program that calls SYS_FORK and reports result via SYS_LOG */
#define SYS_FORK 5
#define SYS_LOG  3
#define SYS_EXIT 1

static inline long user_syscall(long num, long a1, long a2, long a3) {
    long ret;
    asm volatile(
        "mov %1, %%rdi \n\t"
        "mov %2, %%rsi \n\t"
        "mov %3, %%rdx \n\t"
        "mov %4, %%rcx \n\t"
        "int $0x80 \n\t"
        "mov %%rax, %0 \n\t"
        : "=r" (ret)
        : "r" (num), "r" (a1), "r" (a2), "r" (a3)
        : "rdi","rsi","rdx","rcx","rax"
    );
    return ret;
}

void _start(void) {
    long pid = user_syscall(SYS_FORK, 0, 0, 0);
    if (pid == 0) {
        const char msg[] = "[user] fork child says hello\n";
        user_syscall(SYS_LOG, (long)msg, 0, 0);
    } else if (pid > 0) {
        const char msg[] = "[user] fork parent got pid\n";
        user_syscall(SYS_LOG, (long)msg, 0, 0);
    } else {
        const char msg[] = "[user] fork failed\n";
        user_syscall(SYS_LOG, (long)msg, 0, 0);
    }
    user_syscall(SYS_EXIT, 0, 0, 0);
    for (;;) ;
}
