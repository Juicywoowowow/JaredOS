/*
 * =============================================================================
 * SharkOS VGA Text Mode Driver Header (vga.h)
 * =============================================================================
 * VGA text mode (80x25, 16 colors) driver interface.
 *
 * VGA text memory is at 0xB8000. Each character is 2 bytes:
 *   - Byte 0: ASCII character code
 *   - Byte 1: Attribute (foreground color in bits 0-3, background in bits 4-6)
 * =============================================================================
 */

#ifndef VGA_H
#define VGA_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * VGA Text Mode Constants
 * ---------------------------------------------------------------------------- */
#define VGA_WIDTH       80
#define VGA_HEIGHT      25
#define VGA_MEMORY      0xB8000

/* ----------------------------------------------------------------------------
 * VGA Color Codes (4-bit)
 * These can be used for both foreground and background colors.
 * Background colors should be shifted left by 4 bits.
 * ---------------------------------------------------------------------------- */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,  /* Yellow */
    VGA_COLOR_WHITE         = 15
} vga_color_t;

/* ----------------------------------------------------------------------------
 * VGA Functions
 * ---------------------------------------------------------------------------- */

/* Initialize VGA driver (clears screen) */
void vga_init(void);

/* Clear screen with current color */
void vga_clear(void);

/* Set text color (foreground and background) */
void vga_set_color(vga_color_t fg, vga_color_t bg);

/* Get current color attribute byte */
uint8_t vga_get_color(void);

/* Put a single character at current cursor position */
void vga_putchar(char c);

/* Print a null-terminated string */
void vga_print(const char* str);

/* Print an integer (base 10) */
void vga_print_int(int value);

/* Print a hex number (e.g., "0x1F") */
void vga_print_hex(uint32_t value);

/* Set cursor position */
void vga_set_cursor(int x, int y);

/* Get current cursor X position */
int vga_get_cursor_x(void);

/* Get current cursor Y position */
int vga_get_cursor_y(void);

/* Scroll screen up by one line */
void vga_scroll(void);

/* Print a newline (carriage return + line feed) */
void vga_newline(void);

/* Put character at specific position with specific color */
void vga_putchar_at(char c, int x, int y, uint8_t color);

#endif /* VGA_H */
