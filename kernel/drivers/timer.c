/* kernel/drivers/timer.c - minimal timer setup (PIT) */
#include <stdint.h>

#define PIT_FREQ 1193180
#define IRQ0 0
#define TIMER_DIVISOR 1000
/* small outb helper for port I/O */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "dN"(port));
}

void timer_install(void) {
    uint16_t divisor = PIT_FREQ / TIMER_DIVISOR;
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    outb(0x43, 0x34);
    outb(0x40, l);
    outb(0x40, h);
}
