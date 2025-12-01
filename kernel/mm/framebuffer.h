/* kernel/mm/framebuffer.h
 * Simple framebuffer driver for VGA/VESA graphics
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

/* VGA text mode (80x25) */
#define FB_VGA_TEXT_ADDR  0xB8000
#define FB_VGA_TEXT_WIDTH  80
#define FB_VGA_TEXT_HEIGHT 25

/* VGA graphics mode (320x200, 256 colors) - Mode 13h */
#define FB_VGA_GFX_ADDR   0xA0000
#define FB_VGA_GFX_WIDTH  320
#define FB_VGA_GFX_HEIGHT 200

/* Color definitions (VGA 256-color palette) */
#define FB_COLOR_BLACK        0x00
#define FB_COLOR_BLUE         0x01
#define FB_COLOR_GREEN        0x02
#define FB_COLOR_CYAN         0x03
#define FB_COLOR_RED          0x04
#define FB_COLOR_MAGENTA      0x05
#define FB_COLOR_BROWN        0x06
#define FB_COLOR_LIGHT_GRAY   0x07
#define FB_COLOR_DARK_GRAY    0x08
#define FB_COLOR_LIGHT_BLUE   0x09
#define FB_COLOR_LIGHT_GREEN  0x0A
#define FB_COLOR_LIGHT_CYAN   0x0B
#define FB_COLOR_LIGHT_RED    0x0C
#define FB_COLOR_LIGHT_MAGENTA 0x0D
#define FB_COLOR_YELLOW       0x0E
#define FB_COLOR_WHITE        0x0F

/**
 * Initialize framebuffer subsystem
 * Detects available graphics modes and sets up default mode
 */
void fb_init(void);

/**
 * Clear screen with specified color
 * @param color Color index (0-255)
 */
void fb_clear(uint8_t color);

/**
 * Draw a pixel at specified coordinates
 * @param x X coordinate (0-319)
 * @param y Y coordinate (0-199)
 * @param color Color index (0-255)
 */
void fb_putpixel(int x, int y, uint8_t color);

/**
 * Draw a rectangle
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width in pixels
 * @param height Height in pixels
 * @param color Color index (0-255)
 */
void fb_rect(int x, int y, int width, int height, uint8_t color);

/**
 * Draw a filled rectangle
 */
void fb_fill_rect(int x, int y, int width, int height, uint8_t color);

/**
 * Draw a line (Bresenham's algorithm)
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 * @param color Color index (0-255)
 */
void fb_line(int x0, int y0, int x1, int y1, uint8_t color);

/**
 * Draw text at specified position
 * @param x X coordinate (character position, 0-39)
 * @param y Y coordinate (character position, 0-24)
 * @param text String to draw (null-terminated)
 * @param color Foreground color
 * @param bg_color Background color
 */
void fb_text(int x, int y, const char *text, uint8_t color, uint8_t bg_color);

/**
 * Get framebuffer info
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;      // Bits per pixel
    uint64_t addr;     // Physical address
    uint32_t pitch;    // Bytes per line
} fb_info_t;

void fb_get_info(fb_info_t *info);

/* Console functions with scrolling */
void fb_console_putchar(char c);
void fb_console_puts(const char *s);
void fb_console_clear(void);
int fb_console_get_x(void);
int fb_console_get_y(void);
void fb_console_update_cursor(void);
void fb_console_enable_cursor(void);
void fb_console_disable_cursor(void);

#endif /* FRAMEBUFFER_H */
