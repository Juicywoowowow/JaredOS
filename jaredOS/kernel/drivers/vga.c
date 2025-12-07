/**
 * jaredOS - VGA Text Mode Driver Implementation
 */

#include "vga.h"

/* VGA memory address */
#define VGA_MEMORY 0xB8000

/* VGA I/O ports */
#define VGA_CTRL_REG 0x3D4
#define VGA_DATA_REG 0x3D5

/* Current state */
static uint16_t *vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0x0F;  /* White on black */

/**
 * Create VGA entry
 */
static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Update hardware cursor position
 */
static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(VGA_CTRL_REG, 0x0F);
    outb(VGA_DATA_REG, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_REG, 0x0E);
    outb(VGA_DATA_REG, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * Scroll screen up by one line
 */
static void scroll(void) {
    /* Move all lines up */
    for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }

    /* Clear last line */
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = vga_entry(' ', current_color);
    }

    cursor_y = VGA_HEIGHT - 1;
}

/**
 * Initialize VGA driver
 */
void vga_init(void) {
    vga_buffer = (uint16_t*)VGA_MEMORY;
    cursor_x = 0;
    cursor_y = 0;
    current_color = 0x0F;
    vga_enable_cursor();
}

/**
 * Set text color
 */
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = (uint8_t)fg | ((uint8_t)bg << 4);
}

/**
 * Clear screen
 */
void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', current_color);
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

/**
 * Put character at current position
 */
void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(' ', current_color);
        }
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, current_color);
        cursor_x++;
    }

    /* Handle line wrap */
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    /* Handle scroll */
    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }

    update_cursor();
}

/**
 * Print string
 */
void vga_puts(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/**
 * Set cursor position
 */
void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_cursor();
    }
}

/**
 * Get cursor position
 */
void vga_get_cursor(int *x, int *y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

/**
 * Enable cursor
 */
void vga_enable_cursor(void) {
    outb(VGA_CTRL_REG, 0x0A);
    outb(VGA_DATA_REG, (inb(VGA_DATA_REG) & 0xC0) | 14);
    outb(VGA_CTRL_REG, 0x0B);
    outb(VGA_DATA_REG, (inb(VGA_DATA_REG) & 0xE0) | 15);
}

/**
 * Disable cursor
 */
void vga_disable_cursor(void) {
    outb(VGA_CTRL_REG, 0x0A);
    outb(VGA_DATA_REG, 0x20);
}
