/* kernel/drivers/keyboard.c - very small polling keyboard reader (stub) */
#include <stdint.h>

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64
#define KBD_COMMAND_PORT 0x64

static int key_to_char(int scancode) {
    /* very tiny mapping for a few keys */
    if (scancode == 0x1e) return 'a';
    if (scancode == 0x30) return 'b';
    if (scancode == 0x2e) return 'c';
    if (scancode == 0x39) return ' ';
    return 0;
}

/* Wait for keyboard controller to be ready */
static void kbd_wait_input(void) {
    uint8_t status;
    int timeout = 100000;
    do {
        asm volatile("inb %1, %0" : "=a"(status) : "dN"((uint16_t)KBD_STATUS_PORT));
        if (--timeout == 0) break;
    } while (status & 0x02);
}

/* Wait for keyboard controller output */
static void kbd_wait_output(void) {
    uint8_t status;
    int timeout = 100000;
    do {
        asm volatile("inb %1, %0" : "=a"(status) : "dN"((uint16_t)KBD_STATUS_PORT));
        if (--timeout == 0) break;
    } while (!(status & 0x01));
}

int keyboard_getchar(void) {
    unsigned char sc;
    asm volatile ("inb %1, %0" : "=a"(sc) : "dN"(KBD_DATA_PORT));
    int ch = key_to_char(sc);
    return ch ? ch : -1;
}

void keyboard_install(void) {
    /* Initialize keyboard controller */
    
    /* Disable keyboard */
    kbd_wait_input();
    asm volatile("outb %0, %1" :: "a"((uint8_t)0xAD), "dN"((uint16_t)KBD_COMMAND_PORT));
    
    /* Read controller configuration */
    kbd_wait_input();
    asm volatile("outb %0, %1" :: "a"((uint8_t)0x20), "dN"((uint16_t)KBD_COMMAND_PORT));
    kbd_wait_output();
    uint8_t config;
    asm volatile("inb %1, %0" : "=a"(config) : "dN"((uint16_t)KBD_DATA_PORT));
    
    /* Enable keyboard interrupt and scancode translation */
    config |= 0x01;  /* Enable keyboard interrupt */
    config &= ~0x10; /* Enable keyboard */
    
    /* Write controller configuration */
    kbd_wait_input();
    asm volatile("outb %0, %1" :: "a"((uint8_t)0x60), "dN"((uint16_t)KBD_COMMAND_PORT));
    kbd_wait_input();
    asm volatile("outb %0, %1" :: "a"(config), "dN"((uint16_t)KBD_DATA_PORT));
    
    /* Enable keyboard */
    kbd_wait_input();
    asm volatile("outb %0, %1" :: "a"((uint8_t)0xAE), "dN"((uint16_t)KBD_COMMAND_PORT));
    
    /* Flush keyboard buffer */
    kbd_wait_input();
}
