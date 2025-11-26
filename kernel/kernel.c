/* kernel/kernel.c - minimal kernel logic and module glue
 * x86-64 (64-bit) kernel entry point
 * Portabel bootimage for any UEFI/BIOS system (VM, physical hardware, cloud)
 */
#include <stdint.h>

/* forward declarations for functions in other modules */
void gdt_install(void);
void idt_install(void);
void irq_install(void);
void timer_install(void);
void keyboard_install(void);
void serial_init(void);
void paging_enable(void);
void syscall_install(void);

/* ELF loader and process management */
void elf_loader_demo(void);

void show_string(const char *s);

/* entry called from start.S (mbi_ptr passed in EDI for 64-bit signature) */
void kmain(uint32_t mbi_ptr) {
    (void)mbi_ptr; /* Multiboot info not used for now */
    
    /* initialize subsystems */
    serial_init();
    gdt_install();
    idt_install();
    irq_install();
    timer_install();
    keyboard_install();
    
    show_string("=== SOH Descentralizado (64-bit x86-64 Kernel) ===\n");
    show_string("myos: kernel started (portable x86-64 image)\n");

    /* enable paging (identity paging minimal) */
    paging_enable();
    
    /* Setup syscall interface (Phase 1) */
    syscall_install();
    show_string("[kmain] Syscall interface installed\n");
    /* Demo: load and execute embedded user ELF (phase 1 test) */
    elf_loader_demo();

    /* Demo: Test IPC + Scheduler (still functional in 64-bit) */
    extern int task_create(void (*)(void));
    extern void scheduler_start(void);
    extern int ipc_send(const char *s);
    extern int ipc_recv(char *buf, int buflen);

    void producer(void) {
        static int i = 0;
        if (i < 3) {
            ipc_send("hello from producer");
            i++;
        }
    }

    void consumer(void) {
        static int i = 0;
        char buf[128];
        if (i < 3) {
            if (ipc_recv(buf, sizeof(buf)) == 0) {
                show_string("consumer got: ");
                show_string(buf);
                show_string("\n");
            }
            i++;
        }
    }

    /* Create demo tasks */
    task_create(producer);
    task_create(consumer);
    
    show_string("[kmain] Starting cooperative round-robin scheduler...\n");
    scheduler_start();

    /* Should never return */
    for (;;);
}

/* small VGA printer (direct to video memory) */
void show_string(const char *s) {
    /* Send output both to VGA text memory and to serial port so logs
       are visible when running QEMU with `-serial stdio`. */
    volatile char *video = (volatile char*)0xB8000;
    static int pos = 0;
    /* serial_putc is implemented in drivers/serial.c */
    extern void serial_putc(char c);
    while (*s) {
        char ch = *s++;
        video[pos++] = ch;
        video[pos++] = 0x07;
        /* also emit to serial for console logging */
        serial_putc(ch);
        if (pos > 80*25*2-2) pos = 0;
    }
}

/* Implementations for these functions are provided in their respective
    module source files (arch/x86, drivers, etc.). Stubs removed to avoid
    multiple-definition at link time. */

/* irq_install isn't implemented yet in a separate module; provide a
    minimal stub so the kernel can initialize IRQs (replace later with
    a proper implementation). */
/* Setup IRQ controller and unmask essential IRQs (timer) */
void irq_install(void) {
    show_string("IRQ installed\n");
    /* Remap PIC and unmask IRQ0 (timer) so we receive timer interrupts */
    extern void pic_remap(void);
    extern void pic_set_mask(uint8_t mask);

    pic_remap();
    /* Unmask only IRQ0 (timer) for now */
    pic_set_mask(0xFE); /* binary 11111110 -> only IRQ0 unmasked */
}
