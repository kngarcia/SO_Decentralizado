/* kernel/mm/framebuffer.c
 * Simple framebuffer driver implementation
 */

#include "framebuffer.h"
#include "mmio.h"
#include <stdint.h>

extern void show_string(const char *s);

static uint8_t *fb_base = NULL;
static fb_info_t fb_current;

/* VGA mode 13h setup via BIOS is not available in 64-bit protected mode
 * For now, we'll use VGA text mode mapped at 0xB8000
 * or assume GRUB has set up a framebuffer
 */

void fb_init(void) {
    /* For simplicity, use VGA text mode buffer at 0xB8000
     * This is identity-mapped by default in most kernels
     * Direct access without MMIO mapping for now
     */
    
    fb_base = (uint8_t *)FB_VGA_TEXT_ADDR;
    
    fb_current.width = FB_VGA_TEXT_WIDTH;
    fb_current.height = FB_VGA_TEXT_HEIGHT;
    fb_current.bpp = 16;  // 2 bytes per character (char + attribute)
    fb_current.addr = FB_VGA_TEXT_ADDR;
    fb_current.pitch = FB_VGA_TEXT_WIDTH * 2;
    
    /* Clear the screen immediately to remove any garbage */
    uint16_t *fb = (uint16_t *)fb_base;
    uint16_t blank = (0x07 << 8) | ' ';  // Light gray on black, space
    for (int i = 0; i < FB_VGA_TEXT_WIDTH * FB_VGA_TEXT_HEIGHT; i++) {
        fb[i] = blank;
    }
    
    show_string("[fb] Initialized (VGA text mode 80x25, direct @ 0xB8000)\n");
}

void fb_clear(uint8_t color) {
    if (!fb_base) return;
    
    /* For VGA text mode, clear with spaces */
    uint16_t blank = (color << 8) | ' ';
    uint16_t *fb = (uint16_t *)fb_base;
    
    for (int i = 0; i < FB_VGA_TEXT_WIDTH * FB_VGA_TEXT_HEIGHT; i++) {
        fb[i] = blank;
    }
}

void fb_putpixel(int x, int y, uint8_t color) {
    if (!fb_base) return;
    if (x < 0 || x >= (int)fb_current.width) return;
    if (y < 0 || y >= (int)fb_current.height) return;
    
    /* In text mode, we can't do pixel-level graphics
     * Use block characters as a workaround
     */
    uint16_t *fb = (uint16_t *)fb_base;
    int pos = y * FB_VGA_TEXT_WIDTH + x;
    fb[pos] = (color << 8) | 0xDB;  // Use block character █
}

void fb_rect(int x, int y, int width, int height, uint8_t color) {
    /* Draw outline */
    for (int i = 0; i < width; i++) {
        fb_putpixel(x + i, y, color);
        fb_putpixel(x + i, y + height - 1, color);
    }
    for (int i = 0; i < height; i++) {
        fb_putpixel(x, y + i, color);
        fb_putpixel(x + width - 1, y + i, color);
    }
}

void fb_fill_rect(int x, int y, int width, int height, uint8_t color) {
    if (!fb_base) return;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            fb_putpixel(x + i, y + j, color);
        }
    }
}

void fb_line(int x0, int y0, int x1, int y1, uint8_t color) {
    /* Bresenham's line algorithm */
    int dx = x1 - x0;
    int dy = y1 - y0;
    
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        fb_putpixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void fb_text(int x, int y, const char *text, uint8_t color, uint8_t bg_color) {
    if (!fb_base) return;
    if (x < 0 || y < 0 || y >= FB_VGA_TEXT_HEIGHT) return;
    
    uint16_t *fb = (uint16_t *)fb_base;
    uint8_t attrib = (bg_color << 4) | (color & 0x0F);
    int pos = y * FB_VGA_TEXT_WIDTH + x;
    
    for (int i = 0; text[i] != '\0' && (x + i) < FB_VGA_TEXT_WIDTH; i++) {
        fb[pos + i] = (attrib << 8) | text[i];
    }
}

void fb_get_info(fb_info_t *info) {
    if (info) {
        *info = fb_current;
    }
}

/* VGA text console with cursor and scrolling */
static int console_x = 0;
static int console_y = 0;

void fb_console_putchar(char c) {
    if (!fb_base) return;
    
    uint16_t *fb = (uint16_t *)fb_base;
    uint8_t attrib = 0x07;  // Light gray on black (standard VGA text color)
    
    if (c == '\n') {
        console_x = 0;
        console_y++;
        
        // Check for scroll immediately after newline
        if (console_y >= FB_VGA_TEXT_HEIGHT) {
            // Move all lines up
            for (int y = 0; y < FB_VGA_TEXT_HEIGHT - 1; y++) {
                for (int x = 0; x < FB_VGA_TEXT_WIDTH; x++) {
                    int src = (y + 1) * FB_VGA_TEXT_WIDTH + x;
                    int dst = y * FB_VGA_TEXT_WIDTH + x;
                    fb[dst] = fb[src];
                }
            }
            
            // Clear last line
            for (int x = 0; x < FB_VGA_TEXT_WIDTH; x++) {
                int pos = (FB_VGA_TEXT_HEIGHT - 1) * FB_VGA_TEXT_WIDTH + x;
                fb[pos] = (attrib << 8) | ' ';
            }
            
            console_y = FB_VGA_TEXT_HEIGHT - 1;
        }
        fb_console_update_cursor();
    } else if (c == '\r') {
        console_x = 0;
        fb_console_update_cursor();
    } else if (c == '\b') {
        if (console_x > 0) {
            console_x--;
            int pos = console_y * FB_VGA_TEXT_WIDTH + console_x;
            fb[pos] = (attrib << 8) | ' ';
            fb_console_update_cursor();
        }
    } else if (c >= 32) {  // Printable character
        int pos = console_y * FB_VGA_TEXT_WIDTH + console_x;
        fb[pos] = (attrib << 8) | c;
        console_x++;
        
        if (console_x >= FB_VGA_TEXT_WIDTH) {
            console_x = 0;
            console_y++;
            
            // Check for scroll after line wrap
            if (console_y >= FB_VGA_TEXT_HEIGHT) {
                // Move all lines up
                for (int y = 0; y < FB_VGA_TEXT_HEIGHT - 1; y++) {
                    for (int x = 0; x < FB_VGA_TEXT_WIDTH; x++) {
                        int src = (y + 1) * FB_VGA_TEXT_WIDTH + x;
                        int dst = y * FB_VGA_TEXT_WIDTH + x;
                        fb[dst] = fb[src];
                    }
                }
                
                // Clear last line
                for (int x = 0; x < FB_VGA_TEXT_WIDTH; x++) {
                    int pos = (FB_VGA_TEXT_HEIGHT - 1) * FB_VGA_TEXT_WIDTH + x;
                    fb[pos] = (attrib << 8) | ' ';
                }
                
                console_y = FB_VGA_TEXT_HEIGHT - 1;
            }
        }
        fb_console_update_cursor();
    }
}

void fb_console_puts(const char *s) {
    if (!s) return;
    while (*s) {
        fb_console_putchar(*s++);
    }
}

void fb_console_clear(void) {
    if (!fb_base) return;
    
    /* Clear screen with proper VGA text mode format */
    uint16_t *fb = (uint16_t *)fb_base;
    uint16_t blank = (0x07 << 8) | ' ';  // Light gray on black, space
    
    /* Clear entire screen buffer */
    for (int i = 0; i < FB_VGA_TEXT_WIDTH * FB_VGA_TEXT_HEIGHT; i++) {
        fb[i] = blank;
    }
    
    /* Reset cursor position */
    console_x = 0;
    console_y = 0;
    
    /* Update hardware cursor immediately */
    fb_console_update_cursor();
    
    /* Small delay to ensure VGA hardware processes the changes */
    for (volatile int i = 0; i < 1000; i++);
}

int fb_console_get_x(void) {
    return console_x;
}

int fb_console_get_y(void) {
    return console_y;
}

/* VGA hardware cursor control */
void fb_console_update_cursor(void) {
    uint16_t pos = console_y * FB_VGA_TEXT_WIDTH + console_x;
    
    /* Set cursor position using VGA CRT controller registers */
    /* 0x3D4 = CRT Controller Address Register */
    /* 0x3D5 = CRT Controller Data Register */
    
    /* Send high byte */
    asm volatile("outb %0, $0x3D4" :: "a"((uint8_t)0x0E));
    asm volatile("outb %0, $0x3D5" :: "a"((uint8_t)(pos >> 8)));
    
    /* Send low byte */
    asm volatile("outb %0, $0x3D4" :: "a"((uint8_t)0x0F));
    asm volatile("outb %0, $0x3D5" :: "a"((uint8_t)(pos & 0xFF)));
}

void fb_console_enable_cursor(void) {
    /* Enable cursor with scanline 0-15 (full block cursor) */
    asm volatile("outb %0, $0x3D4" :: "a"((uint8_t)0x0A));
    asm volatile("outb %0, $0x3D5" :: "a"((uint8_t)0x00));  /* Cursor start line */
    
    asm volatile("outb %0, $0x3D4" :: "a"((uint8_t)0x0B));
    asm volatile("outb %0, $0x3D5" :: "a"((uint8_t)0x0F));  /* Cursor end line */
}

void fb_console_disable_cursor(void) {
    /* Disable cursor by setting bit 5 of cursor start register */
    asm volatile("outb %0, $0x3D4" :: "a"((uint8_t)0x0A));
    asm volatile("outb %0, $0x3D5" :: "a"((uint8_t)0x20));
}
