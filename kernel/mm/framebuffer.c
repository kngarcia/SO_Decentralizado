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
