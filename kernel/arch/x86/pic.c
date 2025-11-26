/* kernel/arch/x86/pic.c - basic PIC remap and control */
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile ("inb %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

/* PIC ports */
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

void pic_remap(void) {
    /* Save masks */
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    /* start init sequence in cascade mode */
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    /* remap offsets: master->0x20, slave->0x28 */
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    /* setup cascading */
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    /* environment info */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* restore masks */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_set_mask(uint8_t mask) {
    outb(PIC1_DATA, mask);
}

void pic_send_eoi(int irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}
