/* kernel/drivers/serial.c - minimal serial port init and putchar */
#include <stdint.h>
#include "serial.h"

#define SERIAL_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile ("inb %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);    // disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_putc(char c) {
    for (;;) {
        if (inb(SERIAL_PORT + 5) & 0x20) break;
    }
    outb(SERIAL_PORT, (uint8_t)c);
}

void serial_puts(const char *s) {
    if (!s) return;
    while (*s) {
        serial_putc(*s++);
    }
}

void serial_put_hex(uint64_t value) {
    const char *hex = "0123456789abcdef";
    for (int i = 60; i >= 0; i -= 4) {
        serial_putc(hex[(value >> i) & 0xF]);
    }
}

