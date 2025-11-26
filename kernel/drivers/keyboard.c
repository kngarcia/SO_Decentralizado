/* kernel/drivers/keyboard.c - very small polling keyboard reader (stub) */
#include <stdint.h>

#define KBD_DATA_PORT 0x60

static int key_to_char(int scancode) {
    /* very tiny mapping for a few keys */
    if (scancode == 0x1e) return 'a';
    if (scancode == 0x30) return 'b';
    if (scancode == 0x2e) return 'c';
    if (scancode == 0x39) return ' ';
    return 0;
}

int keyboard_getchar(void) {
    unsigned char sc;
    asm volatile ("inb %1, %0" : "=a"(sc) : "dN"(KBD_DATA_PORT));
    int ch = key_to_char(sc);
    return ch ? ch : -1;
}

void keyboard_install(void) {
    /* For demo: no IRQ handler, user can poll keyboard_getchar from tasks */
}
