/**
 * jaredOS - Window Manager Implementation
 * TempleOS-style tiled windows
 */

#include "window.h"
#include "../drivers/vga.h"
#include "../lib/string.h"

/* VGA direct access for window drawing */
#define VGA_MEMORY 0xB8000
static uint16_t *vga = (uint16_t*)VGA_MEMORY;

/* Window buffers */
#define WIN_BUFFER_LINES 50
static uint16_t win0_buffer[WIN_BUFFER_LINES][80];
static uint16_t win1_buffer[WIN_BUFFER_LINES][20];

/* Windows */
static window_t windows[MAX_WINDOWS];
static int active_window = 0;

/* Make VGA entry */
static uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Draw box character at position
 */
static void draw_char(int x, int y, char c, uint8_t color) {
    if (x >= 0 && x < 80 && y >= 0 && y < 25) {
        vga[y * 80 + x] = make_entry(c, color);
    }
}

/**
 * Draw window border
 */
static void draw_border(window_t *w) {
    uint8_t color = w->color;
    
    /* Top border */
    draw_char(w->x, w->y, BOX_TL, color);
    for (int i = 1; i < w->w - 1; i++) {
        draw_char(w->x + i, w->y, BOX_H, color);
    }
    draw_char(w->x + w->w - 1, w->y, BOX_TR, color);
    
    /* Title */
    int title_len = strlen(w->title);
    int title_start = w->x + 2;
    draw_char(title_start - 1, w->y, ' ', color);
    for (int i = 0; i < title_len && i < w->w - 4; i++) {
        draw_char(title_start + i, w->y, w->title[i], color);
    }
    draw_char(title_start + title_len, w->y, ' ', color);
    
    /* Side borders */
    for (int i = 1; i < w->h - 1; i++) {
        draw_char(w->x, w->y + i, BOX_V, color);
        draw_char(w->x + w->w - 1, w->y + i, BOX_V, color);
    }
    
    /* Bottom border */
    draw_char(w->x, w->y + w->h - 1, BOX_BL, color);
    for (int i = 1; i < w->w - 1; i++) {
        draw_char(w->x + i, w->y + w->h - 1, BOX_H, color);
    }
    draw_char(w->x + w->w - 1, w->y + w->h - 1, BOX_BR, color);
}

/**
 * Draw window content from buffer
 */
static void draw_content(window_t *w) {
    if (!w->buffer) return;
    
    uint16_t (*buf)[80] = (uint16_t (*)[80])w->buffer;
    
    for (int row = 0; row < w->content_h; row++) {
        int buf_row = row;  /* Could add scroll_offset here */
        for (int col = 0; col < w->content_w; col++) {
            int vga_x = w->content_x + col;
            int vga_y = w->content_y + row;
            if (buf_row < w->buffer_lines) {
                vga[vga_y * 80 + vga_x] = buf[buf_row][col];
            }
        }
    }
}

/**
 * Initialize window manager with 2-tile layout
 */
void wm_init(void) {
    /* Clear VGA */
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = make_entry(' ', 0x07);
    }
    
    /* Clear buffers */
    for (int i = 0; i < WIN_BUFFER_LINES; i++) {
        for (int j = 0; j < 80; j++) {
            win0_buffer[i][j] = make_entry(' ', 0x07);
        }
        for (int j = 0; j < 20; j++) {
            win1_buffer[i][j] = make_entry(' ', 0x07);
        }
    }
    
    /* Window 0: Main terminal (left, 60 cols) */
    windows[0].x = 0;
    windows[0].y = 0;
    windows[0].w = 60;
    windows[0].h = 25;
    strcpy(windows[0].title, "Terminal");
    windows[0].color = 0x0B;  /* Cyan */
    windows[0].visible = true;
    windows[0].content_x = 1;
    windows[0].content_y = 1;
    windows[0].content_w = 58;
    windows[0].content_h = 23;
    windows[0].cursor_x = 0;
    windows[0].cursor_y = 0;
    windows[0].buffer = (uint16_t*)win0_buffer;
    windows[0].buffer_lines = WIN_BUFFER_LINES;
    windows[0].scroll_offset = 0;
    
    /* Window 1: Status (right, 20 cols) */
    windows[1].x = 59;
    windows[1].y = 0;
    windows[1].w = 21;
    windows[1].h = 25;
    strcpy(windows[1].title, "Status");
    windows[1].color = 0x0A;  /* Green */
    windows[1].visible = true;
    windows[1].content_x = 60;
    windows[1].content_y = 1;
    windows[1].content_w = 19;
    windows[1].content_h = 23;
    windows[1].cursor_x = 0;
    windows[1].cursor_y = 0;
    windows[1].buffer = (uint16_t*)win1_buffer;
    windows[1].buffer_lines = WIN_BUFFER_LINES;
    windows[1].scroll_offset = 0;
    
    active_window = 0;
    
    wm_draw();
}

/**
 * Draw all windows
 */
void wm_draw(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].visible) {
            /* Highlight active window */
            if (i == active_window) {
                windows[i].color = 0x0F;  /* Bright white */
            } else if (i == 0) {
                windows[i].color = 0x0B;  /* Cyan */
            } else {
                windows[i].color = 0x0A;  /* Green */
            }
            draw_border(&windows[i]);
            draw_content(&windows[i]);
        }
    }
}

/**
 * Get active window
 */
window_t* wm_active(void) {
    return &windows[active_window];
}

/**
 * Switch to next window
 */
void wm_next_window(void) {
    active_window = (active_window + 1) % MAX_WINDOWS;
    wm_draw();
}

/**
 * Write character to active window
 */
void wm_putchar(char c) {
    window_t *w = &windows[active_window];
    uint16_t (*buf)[80] = (uint16_t (*)[80])w->buffer;
    
    if (c == '\n') {
        w->cursor_x = 0;
        w->cursor_y++;
    } else if (c == '\r') {
        w->cursor_x = 0;
    } else if (c == '\b') {
        if (w->cursor_x > 0) w->cursor_x--;
    } else {
        if (w->cursor_y < w->buffer_lines && w->cursor_x < w->content_w) {
            buf[w->cursor_y][w->cursor_x] = make_entry(c, 0x07);
            w->cursor_x++;
        }
    }
    
    /* Line wrap */
    if (w->cursor_x >= w->content_w) {
        w->cursor_x = 0;
        w->cursor_y++;
    }
    
    /* Scroll if needed */
    if (w->cursor_y >= w->content_h) {
        /* Shift lines up */
        for (int row = 0; row < w->buffer_lines - 1; row++) {
            for (int col = 0; col < w->content_w; col++) {
                buf[row][col] = buf[row + 1][col];
            }
        }
        /* Clear last line */
        for (int col = 0; col < w->content_w; col++) {
            buf[w->buffer_lines - 1][col] = make_entry(' ', 0x07);
        }
        w->cursor_y = w->content_h - 1;
    }
    
    draw_content(w);
}

/**
 * Write string to active window
 */
void wm_puts(const char *s) {
    while (*s) {
        wm_putchar(*s++);
    }
}

/**
 * Get window by index
 */
window_t* wm_get(int idx) {
    if (idx >= 0 && idx < MAX_WINDOWS) {
        return &windows[idx];
    }
    return NULL;
}

/**
 * Set window title
 */
void wm_set_title(int idx, const char *title) {
    if (idx >= 0 && idx < MAX_WINDOWS) {
        strncpy(windows[idx].title, title, sizeof(windows[idx].title) - 1);
        windows[idx].title[sizeof(windows[idx].title) - 1] = '\0';
    }
}
