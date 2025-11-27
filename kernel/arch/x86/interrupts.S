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
         args in RSI, RDX, RCX. We preserve ALL registers except RAX (return value).
     */

    /* Save ALL caller-saved and callee-saved registers except RAX (will be overwritten by return) */
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

     /* Syscall args: RDI=num, RSI=arg1, RDX=arg2, RCX=arg3
        They're still in registers from user, so pass them directly */
     call syscall_dispatch

     /* Result is in RAX (returned to user). Restore all registers in reverse order.
        Do NOT restore RAX - it contains the syscall return value.
     */
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

        .global isr_0x08
    isr_0x08:
            /* Double Fault - print marker and halt */
            push %rax
            push %rdx
            mov $0x3F8, %dx
            mov $'[', %al
            outb %al, %dx
            mov $'D', %al
            outb %al, %dx
            mov $'F', %al
            outb %al, %dx
            mov $']', %al
            outb %al, %dx
            mov $'\n', %al
            outb %al, %dx
            pop %rdx
            pop %rax
1:          hlt
            jmp 1b

        .global isr_0x0d
    isr_0x0d:
            /* General Protection Fault handler */
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

            /* Pass saved regs ptr in RDI */
            mov %rsp, %rdi
            call general_protection_handler

            /* Restore registers */
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

        .global jump_to_ring3
    jump_to_ring3:
        /* jump_to_ring3(rip, rsp, cs_selector, ss_selector)
           RDI = RIP (user code entry)
           RSI = RSP (user stack pointer)
           RDX = CS selector
           RCX = SS selector
        */
        cli
        
        /* CRITICAL: Ensure kernel stack is 16-byte aligned before building frame */
        and $-16, %rsp
        
        /* Build iretq frame on current kernel stack:
           Stack layout (bottom to top): SS, RSP, RFLAGS, CS, RIP
        */
        pushq %rcx          /* SS (user data selector 0x23) */
        pushq %rsi          /* RSP (user stack) */
        
        /* Set RFLAGS: NO interrupts initially (IF=0), only reserved bit 1 */
        pushq $0x2          /* RFLAGS = 0x2 (IF=0, reserved bit=1) */
        
        pushq %rdx          /* CS (user code selector 0x1B) */
        pushq %rdi          /* RIP (user code entry) */
        
        /* CRITICAL: Set DS and ES to user data selector BEFORE iretq */
        movw %cx, %ds
        movw %cx, %es
        
        /* DEBUG: Print marker before iretq */
        push %rax
        push %rdx
        mov $0x3F8, %dx
        mov $'[', %al
        outb %al, %dx
        mov $'I', %al
        outb %al, %dx
        mov $'R', %al
        outb %al, %dx
        mov $'E', %al
        outb %al, %dx
        mov $'T', %al
        outb %al, %dx
        mov $']', %al
        outb %al, %dx
        mov $0x0A, %al
        outb %al, %dx
        pop %rdx
        pop %rax
        
        /* Execute iretq */
        iretq
