/* kernel/start.S
   Multiboot2 header (UEFI/BIOS compatible) + x86-64 entry point
   Boots on any UEFI/BIOS system (VM, physical hardware, cloud)
*/
    .section .multiboot_header, "aw"
    .align 8
    /* Multiboot2 header */
    .long 0xe85250d6                  /* magic */
    .long 0                           /* architecture (0=32-bit) */
    .long multiboot_header_end - multiboot_header_start
    multiboot_header_start:
    /* checksum */
    .long -(0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start))
    /* end tag */
    .align 8
    .short 0
    .short 0
    .long 8
    multiboot_header_end:

    .code32
    .global _start32
    .type _start32, @function

_start32:
    /* x86 bootloader (GRUB) enters in 32-bit mode.
       We need to switch to x86-64 (64-bit) for full kernel capability.
    */
    cli
     /* Early debug: send marker to COM1 (0x3F8) from 32-bit entry so we can
         confirm we at least entered the boot stub before paging setup. */
     pushl %eax
     movw $0x3F8, %dx
     movb $'S', %al
     outb %al, %dx
     movb $'T', %al
     outb %al, %dx
     movb $'A', %al
     outb %al, %dx
     movb $'R', %al
     outb %al, %dx
     movb $'T', %al
     outb %al, %dx
     movb $0x0A, %al
     outb %al, %dx
     popl %eax

    /* Save multiboot info (EBX = multiboot struct ptr) */
    movl %ebx, mbi_ptr

    /* Debug marker: saved multiboot info */
    pushl %eax
    movw $0x3F8, %dx
    movb $'M', %al
    outb %al, %dx
    movb $'B', %al
    outb %al, %dx
    movb $'I', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx
    popl %eax

    /* Disable paging (if enabled) */
    movl %cr0, %eax
    andl $0x7FFFFFFF, %eax
    movl %eax, %cr0

    /* Debug marker: about to call setup_paging_64 */
    pushl %eax
    movw $0x3F8, %dx
    movb $'B', %al
    outb %al, %dx
    movb $'4', %al
    outb %al, %dx
    movb $'P', %al
    outb %al, %dx
    movb $'G', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx
    popl %eax

    /* Setup 64-bit paging structures (minimal identity map) */
    call setup_paging_64
    /* Debug marker: paging structures set */
    pushl %eax
    movw $0x3F8, %dx
    movb $'P', %al
    outb %al, %dx
    movb $'G', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx
    popl %eax

    /* Enable PAE (Physical Address Extension) */
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    /* Set CR3 to page table */
    leal pml4, %eax
    movl %eax, %cr3
    /* Debug marker: CR3 set */
    pushl %eax
    movw $0x3F8, %dx
    movb $'C', %al
    outb %al, %dx
    movb $'3', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx
    popl %eax

    /* Enable long mode via EFER MSR */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr
    /* Debug marker: EFER written */
    pushl %eax
    movw $0x3F8, %dx
    movb $'E', %al
    outb %al, %dx
    movb $'F', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx
    popl %eax

    /* Enable paging (triggers 64-bit mode) */
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    /* Debug marker: CR0 set -> long mode enabled */
    pushl %eax
    movw $0x3F8, %dx
    movb $'L', %al
    outb %al, %dx
    movb $'M', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx
    popl %eax

    /* Long jump to 64-bit code */
     /*
      * Ensure we have a known-good GDT in place so selector $8 is valid
      * and we won't depend on GRUB's GDT. This reduces chances of
      * triple-faults during the 32->64 transition on uninitialized VMs.
      */
     /* Load a minimal GDT with: NULL, kernel-code (selector 0x08), kernel-data (selector 0x10) */
     lgdt gdt_descriptor

     /* Reload data segment registers so they're consistent with our GDT.
         Use 0x10 (index 2) for data segment. The actual reload of CS happens
         with the long jump below. */
     movw $0x10, %ax
     movw %ax, %ds
     movw %ax, %es
     movw %ax, %ss

     /* Jump into long mode using the code selector 0x08 (index 1 << 3). */
    ljmpl $8, $_start64

    .align 8
    /* Minimal GDT table used for the 32→64 transition. We keep it tiny
     * (NULL, kernel code, kernel data). These descriptors match the flags
     * used later by the kernel's GDT installer. */
    gdt_start:
        .quad 0x0000000000000000    /* NULL descriptor */
        .quad 0x00AF9A000000FFFF    /* kernel code: access=0x9A, gran=0xAF */
        .quad 0x00AF92000000FFFF    /* kernel data: access=0x92, gran=0xAF */
    gdt_end:

    gdt_descriptor:
        .word gdt_end - gdt_start - 1
        .long gdt_start

    .code64
    .global _start64
    .type _start64, @function

_start64:
    /* Now in 64-bit mode */
    /* Early debug: send a short marker to serial port (COM1 0x3F8)
       to confirm we reached 64-bit entry before kmain. This uses
       direct outb instructions so it works before `serial_init`. */
    push %rax
    mov $0x3F8, %dx        /* COM1 */
    mov $'E', %al
    outb %al, %dx
    mov $'A', %al
    outb %al, %dx
    mov $'R', %al
    outb %al, %dx
    mov $'L', %al
    outb %al, %dx
    mov $'Y', %al
    outb %al, %dx
    mov $0x0A, %al          /* newline */
    outb %al, %dx
    pop %rax

    /* Setup 64-bit stack */
    lea stack_top(%rip), %rsp

    /* Zero BSS */
    lea __bss_start(%rip), %rdi
    lea __bss_end(%rip), %rcx
    sub %rdi, %rcx
    xor %al, %al
    rep stosb

    /* Call C kmain (rax = multiboot info ptr as arg) */
    mov mbi_ptr(%rip), %edi
    call kmain

.hang:
    hlt
    jmp .hang

/* Minimal 64-bit paging setup (identity map first 2GB) */
.code32
setup_paging_64:
    /* PML4[0] -> PDPT */
    leal pdpt, %eax
    orl $0x003, %eax              /* present + writeable */
    leal pml4, %edx
    movl %eax, (%edx)

    /* PDPT[0] -> PD */
    leal pd, %eax
    orl $0x003, %eax
    leal pdpt, %edx
    movl %eax, (%edx)

    /* PD[0..511] = 2MB identity map */
    leal pd, %edx
    xor %ecx, %ecx
.loop_pd:
    cmpl $512, %ecx
    je .done_pd
    mov %ecx, %eax
    shl $21, %eax                 /* 2MB per entry */
    orl $0x083, %eax              /* present + writeable + large page */
    movl %eax, (%edx,%ecx,8)
    incl %ecx
    jmp .loop_pd

.done_pd:
    ret

    .section .data
    .align 4
    mbi_ptr: .long 0

    .section .paging, "aw"
    .align 4096
    pml4: .space 4096
    pdpt: .space 4096
    pd:   .space 4096

    .section .bss
    .align 16
    stack_space: .skip 0x8000
    stack_top:
    
    .section .linker_symbols
    __bss_start: .long 0
    __bss_end:   .long 0
