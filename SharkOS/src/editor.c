/*
 * =============================================================================
 * SharkOS Text Editor (editor.c)
 * =============================================================================
 * "SharkVim" - A modal text editor.
 *
 * Modes:
 *   NORMAL: Navigation, saving, quitting
 *   INSERT: Typing text
 * 
 * Commands (Normal Mode):
 *   i      - Enter insert mode
 *   ESC    - Return to normal mode (from insert)
 *   :w     - Save file
 *   :q     - Quit
 *   h/j/k/l- Move cursor (Linux style) or Arrows
 * =============================================================================
 */

#include "editor.h"
#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "memory.h"
#include "string.h"

#define EDIT_BUFFER_SIZE  512
#define STATUS_LINE       24
#define MAX_COLS          80
#define MAX_ROWS          23

typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND
} editor_mode_t;

static char buffer[EDIT_BUFFER_SIZE];
static int cursor_x = 0;
static int cursor_y = 0;
static int content_len = 0;
static char current_filename[32];
static editor_mode_t mode = MODE_NORMAL;
static char cmd_buffer[32];
static int cmd_len = 0;

/* ----------------------------------------------------------------------------
 * clean_buffer - Clear content buffer
 * ---------------------------------------------------------------------------- */
static void clean_buffer(void) {
    memset(buffer, 0, EDIT_BUFFER_SIZE);
    content_len = 0;
    cursor_x = 0;
    cursor_y = 0;
    mode = MODE_NORMAL;
}

/* ----------------------------------------------------------------------------
 * draw_status_bar - Render status line
 * ---------------------------------------------------------------------------- */
static void draw_status_bar(const char* message) {
    int i;
    
    /* Clear status line */
    vga_set_cursor(0, STATUS_LINE);
    vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY); /* Inverted colors */
    
    for (i = 0; i < MAX_COLS; i++) {
        vga_putchar(' ');
    }
    
    vga_set_cursor(0, STATUS_LINE);
    if (mode == MODE_NORMAL) vga_print("NORMAL");
    else if (mode == MODE_INSERT) vga_print("INSERT");
    else if (mode == MODE_COMMAND) {
        vga_print(":");
        vga_print(cmd_buffer);
    }
    
    vga_print(" | ");
    vga_print(current_filename);
    
    if (message) {
        vga_print(" | ");
        vga_print(message);
    }
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK); /* Restore default */
    
    /* In command mode, keep cursor on the status line after the command text */
    if (mode == MODE_COMMAND) {
        /* Position cursor after ':' and command text on status line */
        vga_set_cursor(1 + cmd_len, STATUS_LINE);
    } else {
        vga_set_cursor(cursor_x, cursor_y);
    }
}

/* ----------------------------------------------------------------------------
 * refresh_screen - Redraw buffer content
 * ---------------------------------------------------------------------------- */
static void refresh_screen(void) {
    /* Ensure we're using the correct text color (light grey/white on black) */
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_clear();
    
    /* For simplicity, we just print the raw buffer.
       A real editor would handle line wrapping and scrolling properly. */
    vga_set_cursor(0, 0);
    vga_print(buffer);
    
    draw_status_bar(NULL);
}

/* ----------------------------------------------------------------------------
 * insert_char - Insert character at current position
 * ---------------------------------------------------------------------------- */
static void insert_char(char c) {
    if (content_len >= EDIT_BUFFER_SIZE - 1) return;
    
    /* Calculate linear index */
    /* NOTE: This logic assumes 1D buffer matching screen. Very simplified! */
    /* Real implementation needs line buffer structure. */
    
    buffer[content_len++] = c;
    buffer[content_len] = '\0';
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        cursor_x++;
        if (cursor_x >= MAX_COLS) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    
    refresh_screen();
}

/* ----------------------------------------------------------------------------
 * handle_command - Process ':' commands
 * ---------------------------------------------------------------------------- */
static bool handle_command(void) {
    if (strcmp(cmd_buffer, "w") == 0) {
        /* Save */
        if (fs_exists(current_filename)) {
            fs_delete(current_filename); /* Overwrite */
        }
        
        if (fs_create(current_filename)) {
            fs_write_file(current_filename, (uint8_t*)buffer, content_len);
            draw_status_bar("Saved!");
        } else {
            draw_status_bar("Error: Disk Full");
        }
        return false;
    } else if (strcmp(cmd_buffer, "q") == 0) {
        /* Quit */
        return true;
    } else if (strcmp(cmd_buffer, "wq") == 0) {
        /* Save and Quit */
        if (fs_exists(current_filename)) {
            fs_delete(current_filename);
        }
        if (fs_create(current_filename)) {
            fs_write_file(current_filename, (uint8_t*)buffer, content_len);
            return true;
        } else {
            draw_status_bar("Error: Disk Full");
        }
    } else {
        draw_status_bar("Unknown command");
    }
    return false;
}

/* ----------------------------------------------------------------------------
 * editor_open - Main editor loop
 * ---------------------------------------------------------------------------- */
void editor_open(const char* filename) {
    clean_buffer();
    strcpy(current_filename, filename);
    
    /* Load existing file if present */
    if (fs_exists(filename)) {
        fs_read_file(filename, (uint8_t*)buffer);
        content_len = fs_get_size(filename);
        buffer[content_len] = '\0';
        
        /* Attempt to place cursor at end (rough approx) */
        char* p = buffer;
        while (*p) {
            if (*p == '\n') { cursor_x = 0; cursor_y++; }
            else { cursor_x++; }
            p++;
        }
    }
    
    refresh_screen();
    
    while (1) {
        char c = keyboard_getchar();
        
        /* ------------------------------------------------------- */
        /* COMMAND MODE (:) */
        /* ------------------------------------------------------- */
        if (mode == MODE_COMMAND) {
            if (c == '\n') {
                /* Execute */
                if (handle_command()) break; /* Quit if true */
                mode = MODE_NORMAL;
                cmd_len = 0;
                refresh_screen();
            } else if (c == '\b') {
                if (cmd_len > 0) {
                    cmd_buffer[--cmd_len] = '\0';
                    draw_status_bar(NULL);
                } else {
                    mode = MODE_NORMAL; /* Backspace empty cmd -> Normal */
                    refresh_screen(); /* Redraw content with correct colors */
                }
            } else {
                if (cmd_len < 30) {
                    cmd_buffer[cmd_len++] = c;
                    cmd_buffer[cmd_len] = '\0';
                    draw_status_bar(NULL);
                }
            }
            continue;
        }
        
        /* ------------------------------------------------------- */
        /* INSERT MODE */
        /* ------------------------------------------------------- */
        if (mode == MODE_INSERT) {
            if (c == 27) { /* ESC */
                mode = MODE_NORMAL;
                refresh_screen(); /* Redraw content with correct colors */
            } else if (c == '\b') {
                /* Backspace (simplified: just remove last char from buffer) */
                if (content_len > 0) {
                    content_len--;
                    buffer[content_len] = '\0';
                    
                    /* Move cursor back */
                    if (cursor_x > 0) cursor_x--;
                    else if (cursor_y > 0) {
                        cursor_y--;
                        cursor_x = MAX_COLS - 1; /* Approximate */
                    }
                    refresh_screen();
                }
            } else {
                insert_char(c);
            }
            continue;
        }
        
        /* ------------------------------------------------------- */
        /* NORMAL MODE */
        /* ------------------------------------------------------- */
        if (mode == MODE_NORMAL) {
            switch (c) {
                case 'i': /* Insert mode */
                    mode = MODE_INSERT;
                    draw_status_bar(NULL);
                    break;
                
                case ':': /* Command mode */
                    mode = MODE_COMMAND;
                    cmd_len = 0;
                    cmd_buffer[0] = '\0';
                    draw_status_bar(NULL);
                    break;
                    
                /* Navigation (Vim style + Arrows) */
                /* NOTE: Real arrow keys send scancode sequences.
                   Our keyboard driver simplifies mapping.
                   For now, we'll assume h/j/k/l only. */
                case 'h': if (cursor_x > 0) cursor_x--; break;
                case 'j': if (cursor_y < MAX_ROWS) cursor_y++; break;
                case 'k': if (cursor_y > 0) cursor_y--; break;
                case 'l': if (cursor_x < MAX_COLS) cursor_x++; break;
            }
            vga_set_cursor(cursor_x, cursor_y);
        }
    }
    
    vga_clear();
}
