/*
 * =============================================================================
 * SharkOS VGA Text Mode Driver (vga.c)
 * =============================================================================
 * Implementation of the VGA text mode driver.
 *
 * DEBUGGING TIPS:
 *   - If screen shows garbage, verify you're writing to 0xB8000
 *   - If cursor doesn't move, check the I/O port writes to 0x3D4/0x3D5
 *   - Each cell is 2 bytes: char + attribute. Off-by-one = shifted display
 *   - Scrolling issues often caused by incorrect memcpy offsets
 * =============================================================================
 */

#include "vga.h"
#include "io.h"
#include "memory.h"
#include "string.h"

/* ----------------------------------------------------------------------------
 * VGA I/O Ports for cursor control
 * ---------------------------------------------------------------------------- */
#define VGA_CTRL_PORT   0x3D4
#define VGA_DATA_PORT   0x3D5
#define VGA_CURSOR_HIGH 0x0E
#define VGA_CURSOR_LOW  0x0F

/* ----------------------------------------------------------------------------
 * Static variables
 * ---------------------------------------------------------------------------- */
static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color;

/* ----------------------------------------------------------------------------
 * vga_make_entry - Create a VGA character entry (char + color)
 * ---------------------------------------------------------------------------- */
static inline uint16_t vga_make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* ----------------------------------------------------------------------------
 * vga_make_color - Create color attribute from fg and bg
 * ---------------------------------------------------------------------------- */
static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)fg | ((uint8_t)bg << 4);
}

/* ----------------------------------------------------------------------------
 * update_cursor - Update hardware cursor position via I/O ports
 * 
 * The VGA cursor is controlled by writing to ports 0x3D4 and 0x3D5.
 * Register 0x0E = cursor high byte, register 0x0F = cursor low byte.
 * ---------------------------------------------------------------------------- */
static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    
    /* Write high byte */
    outb(VGA_CTRL_PORT, VGA_CURSOR_HIGH);
    outb(VGA_DATA_PORT, (pos >> 8) & 0xFF);
    
    /* Write low byte */
    outb(VGA_CTRL_PORT, VGA_CURSOR_LOW);
    outb(VGA_DATA_PORT, pos & 0xFF);
}

/* ----------------------------------------------------------------------------
 * vga_init - Initialize VGA driver
 * ---------------------------------------------------------------------------- */
void vga_init(void) {
    /* Set default color: light grey on black */
    current_color = vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Clear screen */
    vga_clear();
    
    /* Reset cursor to top-left */
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

/* ----------------------------------------------------------------------------
 * vga_clear - Clear the entire screen
 * ---------------------------------------------------------------------------- */
void vga_clear(void) {
    uint16_t blank = vga_make_entry(' ', current_color);
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

/* ----------------------------------------------------------------------------
 * vga_set_color - Set current text color
 * ---------------------------------------------------------------------------- */
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    current_color = vga_make_color(fg, bg);
}

/* ----------------------------------------------------------------------------
 * vga_get_color - Get current color attribute
 * ---------------------------------------------------------------------------- */
uint8_t vga_get_color(void) {
    return current_color;
}

/* ----------------------------------------------------------------------------
 * vga_scroll - Scroll screen up by one line
 *
 * Copies lines 1-24 to lines 0-23, then clears line 24.
 * ---------------------------------------------------------------------------- */
void vga_scroll(void) {
    uint16_t blank = vga_make_entry(' ', current_color);
    
    /* Move each line up */
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    /* Clear bottom line */
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
}

/* ----------------------------------------------------------------------------
 * vga_newline - Handle newline (CR + LF)
 * ---------------------------------------------------------------------------- */
void vga_newline(void) {
    cursor_x = 0;
    cursor_y++;
    
    /* Scroll if we've gone past the last line */
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        cursor_y = VGA_HEIGHT - 1;
    }
    
    update_cursor();
}

/* ----------------------------------------------------------------------------
 * vga_putchar - Print a single character at cursor position
 *
 * Handles special characters:
 *   - '\n' : newline
 *   - '\r' : carriage return (move to start of line)
 *   - '\t' : tab (move to next 8-column boundary)
 *   - '\b' : backspace (move cursor back one position)
 * ---------------------------------------------------------------------------- */
void vga_putchar(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }
    
    if (c == '\r') {
        cursor_x = 0;
        update_cursor();
        return;
    }
    
    if (c == '\t') {
        /* Move to next tab stop (every 8 columns) */
        cursor_x = (cursor_x + 8) & ~7;
        if (cursor_x >= VGA_WIDTH) {
            vga_newline();
        } else {
            update_cursor();
        }
        return;
    }
    
    if (c == '\b') {
        /* Backspace - move cursor back and erase character */
        if (cursor_x > 0) {
            cursor_x--;
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_make_entry(' ', current_color);
            update_cursor();
        }
        return;
    }
    
    /* Regular character - write to buffer and advance cursor */
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_make_entry(c, current_color);
    cursor_x++;
    
    /* Wrap to next line if needed */
    if (cursor_x >= VGA_WIDTH) {
        vga_newline();
    } else {
        update_cursor();
    }
}

/* ----------------------------------------------------------------------------
 * vga_putchar_at - Put character at specific position with specific color
 * ---------------------------------------------------------------------------- */
void vga_putchar_at(char c, int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_buffer[y * VGA_WIDTH + x] = vga_make_entry(c, color);
    }
}

/* ----------------------------------------------------------------------------
 * vga_print - Print a null-terminated string
 * ---------------------------------------------------------------------------- */
void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

/* ----------------------------------------------------------------------------
 * vga_print_int - Print an integer in base 10
 * ---------------------------------------------------------------------------- */
void vga_print_int(int value) {
    char buf[16];
    itoa(value, buf, 10);
    vga_print(buf);
}

/* ----------------------------------------------------------------------------
 * vga_print_hex - Print a value in hexadecimal with "0x" prefix
 * ---------------------------------------------------------------------------- */
void vga_print_hex(uint32_t value) {
    char buf[16];
    vga_print("0x");
    
    /* Special case for zero */
    if (value == 0) {
        vga_print("0");
        return;
    }
    
    /* Convert using itoa with base 16 */
    char* ptr = buf;
    while (value != 0) {
        int digit = value % 16;
        *ptr++ = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= 16;
    }
    *ptr = '\0';
    
    /* Reverse and print */
    ptr--;
    while (ptr >= buf) {
        vga_putchar(*ptr--);
    }
}

/* ----------------------------------------------------------------------------
 * vga_set_cursor - Set cursor position
 * ---------------------------------------------------------------------------- */
void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_cursor();
    }
}

/* ----------------------------------------------------------------------------
 * vga_get_cursor_x - Get current cursor X position
 * ---------------------------------------------------------------------------- */
int vga_get_cursor_x(void) {
    return cursor_x;
}

/* ----------------------------------------------------------------------------
 * vga_get_cursor_y - Get current cursor Y position
 * ---------------------------------------------------------------------------- */
int vga_get_cursor_y(void) {
    return cursor_y;
}
