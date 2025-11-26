/* user/libc/minimal.c - tiny libc-like helpers usable by init/user programs */
#include <stdarg.h>
#include <stdint.h>

int puts(const char *s) {
    volatile char *video = (volatile char*)0xB8000;
    static int pos = 80*24*2;
    while (*s) {
        video[pos++] = *s++;
        video[pos++] = 0x07;
    }
    return 0;
}
