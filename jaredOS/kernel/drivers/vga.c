/**
 * jaredOS - VGA Text Mode Driver Implementation
 */

#include "vga.h"

/* VGA memory address */
#define VGA_MEMORY 0xB8000

/* VGA I/O ports */
#define VGA_CTRL_REG 0x3D4
#define VGA_DATA_REG 0x3D5

/* Scrollback buffer - stores all history */
static uint16_t scrollback[VGA_SCROLLBACK_LINES][VGA_WIDTH];
static int buffer_line = 0;      /* Current line in buffer (where new text goes) */
static int view_offset = 0;      /* How many lines we've scrolled up (0 = at bottom) */
static int total_lines = 0;      /* Total lines written so far */

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
 * Refresh VGA from scrollback buffer
 */
static void refresh_display(void) {
    int view_start = buffer_line - VGA_HEIGHT - view_offset + 1;
    if (view_start < 0) view_start = 0;
    
    for (int row = 0; row < VGA_HEIGHT; row++) {
        int buf_line = (view_start + row) % VGA_SCROLLBACK_LINES;
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[row * VGA_WIDTH + col] = scrollback[buf_line][col];
        }
    }
    update_cursor();
}

/**
 * Scroll buffer up by one line (new line added)
 */
static void scroll_buffer(void) {
    /* Move to next line in circular buffer */
    buffer_line = (buffer_line + 1) % VGA_SCROLLBACK_LINES;
    total_lines++;
    
    /* Clear the new line */
    for (int i = 0; i < VGA_WIDTH; i++) {
        scrollback[buffer_line][i] = vga_entry(' ', current_color);
    }
    
    /* Reset view to bottom */
    view_offset = 0;
}

/**
 * Initialize VGA driver
 */
void vga_init(void) {
    vga_buffer = (uint16_t*)VGA_MEMORY;
    cursor_x = 0;
    cursor_y = 0;
    buffer_line = VGA_HEIGHT - 1;  /* Start at bottom of initial screen */
    view_offset = 0;
    total_lines = VGA_HEIGHT;
    current_color = 0x0F;
    
    /* Clear scrollback buffer */
    for (int line = 0; line < VGA_SCROLLBACK_LINES; line++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            scrollback[line][col] = vga_entry(' ', current_color);
        }
    }
    
    vga_enable_cursor();
    refresh_display();
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
    /* Clear scrollback and VGA */
    for (int line = 0; line < VGA_SCROLLBACK_LINES; line++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            scrollback[line][col] = vga_entry(' ', current_color);
        }
    }
    buffer_line = VGA_HEIGHT - 1;
    view_offset = 0;
    total_lines = VGA_HEIGHT;
    cursor_x = 0;
    cursor_y = 0;
    refresh_display();
}

/**
 * Put character at current position
 */
void vga_putchar(char c) {
    /* Auto-scroll to bottom on new output */
    if (view_offset != 0) {
        view_offset = 0;
    }
    
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
            int line = (buffer_line - (VGA_HEIGHT - 1 - cursor_y) + VGA_SCROLLBACK_LINES) % VGA_SCROLLBACK_LINES;
            scrollback[line][cursor_x] = vga_entry(' ', current_color);
        }
    } else {
        int line = (buffer_line - (VGA_HEIGHT - 1 - cursor_y) + VGA_SCROLLBACK_LINES) % VGA_SCROLLBACK_LINES;
        scrollback[line][cursor_x] = vga_entry(c, current_color);
        cursor_x++;
    }

    /* Handle line wrap */
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    /* Handle scroll */
    if (cursor_y >= VGA_HEIGHT) {
        scroll_buffer();
        cursor_y = VGA_HEIGHT - 1;
    }

    refresh_display();
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

/**
 * Scroll view up (see older lines)
 */
void vga_scroll_up(void) {
    int max_offset = total_lines - VGA_HEIGHT;
    if (max_offset < 0) max_offset = 0;
    if (view_offset < max_offset) {
        view_offset++;
        refresh_display();
    }
}

/**
 * Scroll view down (see newer lines)
 */
void vga_scroll_down(void) {
    if (view_offset > 0) {
        view_offset--;
        refresh_display();
    }
}

/**
 * Scroll to bottom (current output)
 */
void vga_scroll_to_bottom(void) {
    if (view_offset != 0) {
        view_offset = 0;
        refresh_display();
    }
}
