/* kernel/arch/x86/interrupts.S - gdt_flush and idt_flush (x86-64) + simple isr stubs */

    .code64
    
    .global gdt_flush
gdt_flush:
    /* RDI = address of GDT ptr */
    lgdt (%rdi)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    /* Long jump to flush CPU pipeline (use retq as workaround for 64-bit) */
    retq

    .global idt_flush
idt_flush:
    /* RDI = address of IDT ptr */
    lidt (%rdi)
    retq

     .global isr_0x80
isr_0x80:
     /* Syscall software interrupt handler (vector 0x80)
         Calling convention: user places syscall number in RDI,
         args in RSI, RDX, RCX. We preserve callee-saved registers
         and call C dispatcher syscall_dispatch(num, arg1, arg2, arg3).
     */
     /* Save callee-saved registers */
     pushq %rbp
     pushq %rbx
     pushq %r12
     pushq %r13
     pushq %r14
     pushq %r15

     /* Ensure stack 16-byte alignment before call (interrupt entry may be aligned)
         Call syscall_dispatch directly; user-provided args are already in RDI, RSI, RDX, RCX.
     */
     call syscall_dispatch

     /* Result is in RAX (returned to user). Restore callee-saved registers (do not clobber RAX)
         We must restore in reverse order. Note: we didn't save RAX so it contains result.
     */
     popq %r15
     popq %r14
     popq %r13
     popq %r12
     popq %rbx
     popq %rbp

     iretq
