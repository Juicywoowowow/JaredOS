/**
 * jaredOS - Simple Text Editor
 */

#include "editor.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../lib/string.h"
#include "../lib/printf.h"

/* Editor buffer */
static char buffer[EDITOR_MAX_LINES][EDITOR_MAX_COLS + 1];
static int cursor_x = 0;
static int cursor_y = 0;
static int num_lines = 1;
static bool modified = false;
static char current_filename[64];

/**
 * Draw status bar at bottom
 */
static void draw_status(void) {
    vga_set_cursor(0, 24);
    vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
    
    kprintf(" ");
    if (current_filename[0]) {
        kprintf("%s", current_filename);
    } else {
        kprintf("[New File]");
    }
    if (modified) kprintf(" *");
    
    /* Pad and show controls */
    int pos = strlen(current_filename) + (modified ? 3 : 1) + 1;
    for (int i = pos; i < 50; i++) vga_putchar(' ');
    kprintf("Ctrl+S: Save  Ctrl+Q: Quit  L:%d C:%d", cursor_y + 1, cursor_x + 1);
    
    /* Pad rest */
    int x, y;
    vga_get_cursor(&x, &y);
    for (int i = x; i < 80; i++) vga_putchar(' ');
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/**
 * Draw a single line
 */
static void draw_line(int line) {
    vga_set_cursor(0, line);
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    
    /* Line number */
    if (line + 1 < 10) kprintf(" ");
    kprintf("%d ", line + 1);
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Line content */
    int len = strlen(buffer[line]);
    for (int i = 0; i < len && i < EDITOR_MAX_COLS - 4; i++) {
        vga_putchar(buffer[line][i]);
    }
    
    /* Clear rest of line */
    int x, y;
    vga_get_cursor(&x, &y);
    for (int i = x; i < 80; i++) vga_putchar(' ');
}

/**
 * Redraw entire editor
 */
static void redraw(void) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Draw title bar */
    vga_set_cursor(0, 0);
    vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
    kprintf("  jaredOS Editor                                                              ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Draw lines (lines 1-23 on screen) */
    for (int i = 0; i < EDITOR_MAX_LINES; i++) {
        vga_set_cursor(0, i + 1);
        draw_line(i);
    }
    
    draw_status();
    
    /* Position cursor */
    vga_set_cursor(cursor_x + 3, cursor_y + 1);
}

/**
 * Handle input in editor
 */
static bool handle_input(void) {
    char c = keyboard_getchar();
    
    /* Ctrl+Q - Quit */
    if (c == 17) {  /* Ctrl+Q */
        return false;
    }
    
    /* Ctrl+S - Save (placeholder) */
    if (c == 19) {  /* Ctrl+S */
        /* Will be connected to filesystem later */
        vga_set_cursor(0, 24);
        vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
        kprintf("  Saved! (to memory)                                                          ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        modified = false;
        return true;
    }
    
    /* Enter - New line */
    if (c == '\n') {
        if (num_lines < EDITOR_MAX_LINES) {
            /* Shift lines down */
            for (int i = num_lines; i > cursor_y + 1; i--) {
                strcpy(buffer[i], buffer[i - 1]);
            }
            /* Split current line */
            strcpy(buffer[cursor_y + 1], &buffer[cursor_y][cursor_x]);
            buffer[cursor_y][cursor_x] = '\0';
            num_lines++;
            cursor_y++;
            cursor_x = 0;
            modified = true;
            redraw();
        }
        return true;
    }
    
    /* Backspace */
    if (c == '\b') {
        if (cursor_x > 0) {
            int len = strlen(buffer[cursor_y]);
            for (int i = cursor_x - 1; i < len; i++) {
                buffer[cursor_y][i] = buffer[cursor_y][i + 1];
            }
            cursor_x--;
            modified = true;
            draw_line(cursor_y);
            draw_status();
            vga_set_cursor(cursor_x + 3, cursor_y + 1);
        } else if (cursor_y > 0) {
            /* Join with previous line */
            int prev_len = strlen(buffer[cursor_y - 1]);
            strcat(buffer[cursor_y - 1], buffer[cursor_y]);
            /* Shift lines up */
            for (int i = cursor_y; i < num_lines - 1; i++) {
                strcpy(buffer[i], buffer[i + 1]);
            }
            buffer[num_lines - 1][0] = '\0';
            num_lines--;
            cursor_y--;
            cursor_x = prev_len;
            modified = true;
            redraw();
        }
        return true;
    }
    
    /* Regular character */
    if (c >= 32 && c < 127) {
        int len = strlen(buffer[cursor_y]);
        if (len < EDITOR_MAX_COLS - 4) {
            /* Shift characters right */
            for (int i = len + 1; i > cursor_x; i--) {
                buffer[cursor_y][i] = buffer[cursor_y][i - 1];
            }
            buffer[cursor_y][cursor_x] = c;
            cursor_x++;
            modified = true;
            draw_line(cursor_y);
            draw_status();
            vga_set_cursor(cursor_x + 3, cursor_y + 1);
        }
        return true;
    }
    
    return true;
}

/**
 * Open editor
 */
void editor_open(const char *filename) {
    /* Clear buffer */
    for (int i = 0; i < EDITOR_MAX_LINES; i++) {
        memset(buffer[i], 0, EDITOR_MAX_COLS + 1);
    }
    
    cursor_x = 0;
    cursor_y = 0;
    num_lines = 1;
    modified = false;
    
    if (filename) {
        strncpy(current_filename, filename, 63);
        current_filename[63] = '\0';
    } else {
        current_filename[0] = '\0';
    }
    
    vga_clear();
    redraw();
    
    /* Main editor loop */
    while (handle_input()) {
        /* Continue until quit */
    }
    
    /* Restore screen */
    vga_clear();
    kprintf("Editor closed.\n\n");
}

/**
 * Get buffer for saving
 */
const char* editor_get_buffer(void) {
    static char flat_buffer[EDITOR_BUFFER_SIZE];
    int pos = 0;
    
    for (int i = 0; i < num_lines; i++) {
        int len = strlen(buffer[i]);
        memcpy(&flat_buffer[pos], buffer[i], len);
        pos += len;
        flat_buffer[pos++] = '\n';
    }
    flat_buffer[pos] = '\0';
    
    return flat_buffer;
}
