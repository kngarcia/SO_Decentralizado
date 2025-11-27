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

        .global isr_0x20
isr_0x20:
        /* Timer IRQ (IRQ0) handler for preemptive scheduler
             Stack layout when entry comes here (CPU pushed RIP,CS,RFLAGS):
                - [RIP]   (at higher address)
                - [CS]
                - [RFLAGS]
            We push general registers so the C scheduler can save/restore them.
        */
        /* Save general purpose registers (order chosen so pop restores correctly) */
        pushq %rax
        pushq %rcx
        pushq %rdx
        pushq %rsi
        pushq %rdi
        pushq %r8
        pushq %r9
        pushq %r10
        pushq %r11
        pushq %rbx
        pushq %rbp
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15

        /* Pass current RSP (pointer to saved regs) to C scheduler_tick
             The scheduler will return the RSP for the next task to run in RAX.
        */
        mov %rsp, %rdi
        call scheduler_tick

        /* scheduler_tick returns next_rsp in RAX - use it */
        mov %rax, %rsp

        /* Restore registers (reverse of push order) */
        popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %rbp
        popq %rbx
        popq %r11
        popq %r10
        popq %r9
        popq %r8
        popq %rdi
        popq %rsi
        popq %rdx
        popq %rcx
        popq %rax

        iretq

        .global isr_0x0e
    isr_0x0e:
            /* Page Fault handler: push general registers then call C handler
               We pass pointer to saved registers in RDI and CR2 (fault address) in RSI
            */
            pushq %rax
            pushq %rcx
            pushq %rdx
            pushq %rsi
            pushq %rdi
            pushq %r8
            pushq %r9
            pushq %r10
            pushq %r11
            pushq %rbx
            pushq %rbp
            pushq %r12
            pushq %r13
            pushq %r14
            pushq %r15

            /* Pass parameters: rdi = saved regs ptr (rsp), rsi = CR2 (fault addr) */
            mov %rsp, %rdi
            mov %cr2, %rsi
            call page_fault_handler

            /* page_fault_handler returns next_rsp in RAX; set RSP to it */
            mov %rax, %rsp

            /* Restore registers (reverse of push order) */
            popq %r15
            popq %r14
            popq %r13
            popq %r12
            popq %rbp
            popq %rbx
            popq %r11
            popq %r10
            popq %r9
            popq %r8
            popq %rdi
            popq %rsi
            popq %rdx
            popq %rcx
            popq %rax

            iretq

        .global enable_interrupts
    enable_interrupts:
        sti
        retq

        .global disable_interrupts
    disable_interrupts:
        cli
        retq
