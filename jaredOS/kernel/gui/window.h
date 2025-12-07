/**
 * jaredOS - Window Manager Header
 * TempleOS-style tiled windows in text mode
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "../types.h"

/* Maximum tiles */
#define MAX_WINDOWS 2

/* Box drawing characters (CP437) */
#define BOX_H       0xC4  /* ─ horizontal */
#define BOX_V       0xB3  /* │ vertical */
#define BOX_TL      0xDA  /* ┌ top-left */
#define BOX_TR      0xBF  /* ┐ top-right */
#define BOX_BL      0xC0  /* └ bottom-left */
#define BOX_BR      0xD9  /* ┘ bottom-right */
#define BOX_LT      0xC3  /* ├ left-tee */
#define BOX_RT      0xB4  /* ┤ right-tee */
#define BOX_TT      0xC2  /* ┬ top-tee */
#define BOX_BT      0xC1  /* ┴ bottom-tee */
#define BOX_X       0xC5  /* ┼ cross */

/* Window structure */
typedef struct {
    int x, y;           /* Position */
    int w, h;           /* Size (including borders) */
    char title[20];     /* Window title */
    uint8_t color;      /* Border color */
    bool visible;       /* Is window visible */
    
    /* Content area (inside borders) */
    int content_x, content_y;
    int content_w, content_h;
    int cursor_x, cursor_y;
    
    /* Scrollback for this window */
    uint16_t *buffer;   /* Points to scrollback buffer */
    int buffer_lines;
    int scroll_offset;
} window_t;

/* Initialize window manager */
void wm_init(void);

/* Draw all windows */
void wm_draw(void);

/* Get active window */
window_t* wm_active(void);

/* Switch active window (Tab key) */
void wm_next_window(void);

/* Write to active window */
void wm_putchar(char c);
void wm_puts(const char *s);

/* Get window by index */
window_t* wm_get(int idx);

/* Set window title */
void wm_set_title(int idx, const char *title);

#endif /* WINDOW_H */
