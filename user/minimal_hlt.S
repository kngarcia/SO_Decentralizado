.section .text
.global _start
_start:
    # Infinite loop with nop (user-safe)
1:  nop
    jmp 1b
