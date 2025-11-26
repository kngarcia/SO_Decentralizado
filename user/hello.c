/* user/hello.c - minimal user-mode program that uses int 0x80 syscalls */

/* Syscall numbers must match kernel/syscall.h */
#define SYS_EXIT 1
#define SYS_LOG  3

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
    const char msg[] = "[user] Hello from ring-3!\n";
    user_syscall(SYS_LOG, (long)msg, 0, 0);
    user_syscall(SYS_EXIT, 0, 0, 0);
    for (;;) ;
}
