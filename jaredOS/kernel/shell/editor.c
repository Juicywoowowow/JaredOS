/**
 * jaredOS - Simple Text Editor (Simplified)
 */

#include "editor.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../fs/simplefs.h"
#include "../lib/string.h"

/* Editor state */
#define MAX_LINES 22
#define MAX_COLS  76

static char lines[MAX_LINES][MAX_COLS + 1];
static int cur_line = 0;
static int cur_col = 0;
static int total_lines = 1;
static bool dirty = false;
static char filename[32];

/* Direct VGA access to avoid scrolling */
#define VGA_MEM ((uint16_t*)0xB8000)

static void editor_putchar_at(int x, int y, char c, uint8_t color) {
    if (x >= 0 && x < 80 && y >= 0 && y < 25) {
        VGA_MEM[y * 80 + x] = (uint16_t)c | ((uint16_t)color << 8);
    }
}

static void editor_clear_line(int y, uint8_t color) {
    for (int x = 0; x < 80; x++) {
        editor_putchar_at(x, y, ' ', color);
    }
}

static void editor_print_at(int x, int y, const char *s, uint8_t color) {
    while (*s && x < 80) {
        editor_putchar_at(x++, y, *s++, color);
    }
}

/* Draw title bar */
static void draw_title(void) {
    uint8_t title_color = 0x3F;  /* White on cyan */
    editor_clear_line(0, title_color);
    editor_print_at(2, 0, "jaredOS Editor", title_color);
    if (filename[0]) {
        editor_print_at(20, 0, filename, title_color);
    }
    if (dirty) {
        editor_print_at(52, 0, "[Modified]", title_color);
    }
}

/* Draw status bar */  
static void draw_status(void) {
    uint8_t status_color = 0x70;  /* Black on white */
    editor_clear_line(24, status_color);
    editor_print_at(2, 24, "^Q:Quit  ^S:Save", status_color);
    
    /* Show line/col */
    char pos[20];
    int i = 0;
    pos[i++] = 'L';
    pos[i++] = ':';
    int ln = cur_line + 1;
    if (ln >= 10) pos[i++] = '0' + (ln / 10);
    pos[i++] = '0' + (ln % 10);
    pos[i++] = ' ';
    pos[i++] = 'C';
    pos[i++] = ':';
    int col = cur_col + 1;
    if (col >= 10) pos[i++] = '0' + (col / 10);
    pos[i++] = '0' + (col % 10);
    pos[i] = 0;
    editor_print_at(70, 24, pos, status_color);
}

/* Draw a single editor line */
static void draw_editor_line(int line) {
    int screen_y = line + 1;  /* Lines 1-22 are editor content */
    uint8_t text_color = 0x0F;  /* White on black */
    uint8_t num_color = 0x08;   /* Dark grey */
    
    editor_clear_line(screen_y, text_color);
    
    /* Line number */
    int ln = line + 1;
    editor_putchar_at(0, screen_y, '0' + (ln / 10), num_color);
    editor_putchar_at(1, screen_y, '0' + (ln % 10), num_color);
    editor_putchar_at(2, screen_y, ' ', num_color);
    
    /* Line content */
    int col = 0;
    while (lines[line][col] && col < MAX_COLS) {
        editor_putchar_at(col + 3, screen_y, lines[line][col], text_color);
        col++;
    }
}

/* Draw empty line marker */
static void draw_empty_line(int line) {
    int screen_y = line + 1;
    uint8_t color = 0x08;  /* Dark grey */
    editor_clear_line(screen_y, 0x0F);
    editor_putchar_at(0, screen_y, '~', color);
}

/* Full redraw */
static void redraw_all(void) {
    draw_title();
    for (int i = 0; i < MAX_LINES; i++) {
        if (i < total_lines) {
            draw_editor_line(i);
        } else {
            draw_empty_line(i);
        }
    }
    draw_status();
}

/* Position hardware cursor */
static void update_cursor(void) {
    int x = cur_col + 3;
    int y = cur_line + 1;
    uint16_t pos = y * 80 + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
}

/* Handle a keypress */
static bool handle_key(char c) {
    /* Ctrl+Q = quit (code 17) */
    if (c == 17) {
        return false;
    }
    
    /* Ctrl+S = save (code 19) */
    if (c == 19) {
        if (filename[0] && fs_ready()) {
            const char *buf = editor_get_buffer();
            if (fs_write(filename, buf, strlen(buf))) {
                editor_print_at(2, 24, "Saved to disk!          ", 0x2F);
                dirty = false;
            } else {
                editor_print_at(2, 24, "Save failed!            ", 0x4F);
            }
        } else if (!filename[0]) {
            editor_print_at(2, 24, "No filename!            ", 0x4F);
        } else {
            editor_print_at(2, 24, "No filesystem! Use format", 0x4F);
        }
        return true;
    }
    
    /* Enter */
    if (c == '\n') {
        if (total_lines < MAX_LINES) {
            /* Move lines down */
            for (int i = total_lines; i > cur_line + 1; i--) {
                strcpy(lines[i], lines[i - 1]);
            }
            /* Split line */
            strcpy(lines[cur_line + 1], &lines[cur_line][cur_col]);
            lines[cur_line][cur_col] = '\0';
            total_lines++;
            cur_line++;
            cur_col = 0;
            dirty = true;
            redraw_all();
        }
        return true;
    }
    
    /* Backspace */
    if (c == '\b') {
        if (cur_col > 0) {
            int len = strlen(lines[cur_line]);
            for (int i = cur_col - 1; i < len; i++) {
                lines[cur_line][i] = lines[cur_line][i + 1];
            }
            cur_col--;
            dirty = true;
            draw_editor_line(cur_line);
            draw_status();
            update_cursor();
        } else if (cur_line > 0) {
            /* Join with previous line */
            int prev_len = strlen(lines[cur_line - 1]);
            strcat(lines[cur_line - 1], lines[cur_line]);
            for (int i = cur_line; i < total_lines - 1; i++) {
                strcpy(lines[i], lines[i + 1]);
            }
            lines[total_lines - 1][0] = '\0';
            total_lines--;
            cur_line--;
            cur_col = prev_len;
            dirty = true;
            redraw_all();
        }
        return true;
    }
    
    /* Printable character */
    if (c >= 32 && c < 127) {
        int len = strlen(lines[cur_line]);
        if (len < MAX_COLS - 1) {
            /* Insert char */
            for (int i = len + 1; i > cur_col; i--) {
                lines[cur_line][i] = lines[cur_line][i - 1];
            }
            lines[cur_line][cur_col] = c;
            cur_col++;
            dirty = true;
            draw_editor_line(cur_line);
            draw_title();
            draw_status();
            update_cursor();
        }
        return true;
    }
    
    return true;
}

/* Public: Open editor */
void editor_open(const char *fname) {
    /* Clear state */
    for (int i = 0; i < MAX_LINES; i++) {
        memset(lines[i], 0, MAX_COLS + 1);
    }
    cur_line = 0;
    cur_col = 0;
    total_lines = 1;
    dirty = false;
    
    if (fname) {
        strncpy(filename, fname, 31);
        filename[31] = '\0';
        
        /* Try to load file from disk */
        if (fs_ready()) {
            static char load_buf[MAX_LINES * (MAX_COLS + 2)];
            int bytes = fs_read(fname, load_buf, sizeof(load_buf) - 1);
            if (bytes > 0) {
                load_buf[bytes] = '\0';
                /* Parse into lines */
                int line = 0;
                int col = 0;
                for (int i = 0; i < bytes && line < MAX_LINES; i++) {
                    if (load_buf[i] == '\n') {
                        lines[line][col] = '\0';
                        line++;
                        col = 0;
                    } else if (col < MAX_COLS) {
                        lines[line][col++] = load_buf[i];
                    }
                }
                if (col > 0) line++;  /* Count partial last line */
                total_lines = (line > 0) ? line : 1;
            }
        }
    } else {
        filename[0] = '\0';
    }
    
    /* Clear screen and draw */
    for (int y = 0; y < 25; y++) {
        editor_clear_line(y, 0x0F);
    }
    redraw_all();
    update_cursor();
    
    /* Main loop */
    while (1) {
        char c = keyboard_getchar();
        if (!handle_key(c)) break;
        update_cursor();
    }
    
    /* Restore normal screen */
    vga_clear();
}

/* Public: Get buffer */
const char* editor_get_buffer(void) {
    static char buf[MAX_LINES * (MAX_COLS + 2)];
    int pos = 0;
    for (int i = 0; i < total_lines; i++) {
        int len = strlen(lines[i]);
        memcpy(&buf[pos], lines[i], len);
        pos += len;
        buf[pos++] = '\n';
    }
    buf[pos] = '\0';
    return buf;
}
