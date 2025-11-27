.section .text
.global _start
_start:
    # Just many NOPs, no jumps
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    # After 16 NOPs, try int 0x80 (syscall)
    mov $1, %rax      # sys_exit
    int $0x80
