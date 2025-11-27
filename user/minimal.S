# Minimal user-mode test program
# Just halt immediately to test ring-3 transition
    .global _start
    .section .text
_start:
    # Do nothing, just halt
    hlt
    jmp _start
