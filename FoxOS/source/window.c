/*
 * =============================================================================
 * window.c - Window Manager for FoxOS
 * =============================================================================
 *
 * This file implements a simple window manager with:
 *   - Draggable windows with title bars
 *   - Close buttons
 *   - Focus management (click to focus)
 *   - Z-ordering (focused window on top)
 *
 * DEBUGGING TIPS:
 *   - If windows don't appear, check vga_init was called
 *   - If dragging doesn't work, verify mouse_handler is firing
 *   - If clicks don't register, check mouse position bounds
 *
 * =============================================================================
 */

#include "types.h"

/* External functions */
extern void vga_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                          uint8_t color);
extern void vga_draw_rect_outline(int32_t x, int32_t y, int32_t w, int32_t h,
                                  uint8_t color);
extern void vga_draw_button(int32_t x, int32_t y, int32_t w, int32_t h,
                            bool pressed);
extern void font_draw_string(int32_t x, int32_t y, const char *str, uint8_t fg,
                             uint8_t bg);
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern bool mouse_is_left_pressed(void);
extern void *kmalloc(uint32_t size);
extern void kfree(void *ptr);

/* =============================================================================
 * SECTION 1: Window Constants and Colors
 * =============================================================================
 */

#define MAX_WINDOWS 16
#define TITLE_BAR_HEIGHT 14
#define CLOSE_BTN_SIZE 10
#define BORDER_WIDTH 2

/* Colors */
#define COLOR_WINDOW_BG 17
#define COLOR_TITLE_BAR 18
#define COLOR_TITLE_TEXT 15
#define COLOR_BORDER 8
#define COLOR_CLOSE_BTN 4

/* =============================================================================
 * SECTION 2: Window Structure
 * =============================================================================
 */

typedef struct {
  int32_t x, y;          /* Position */
  int32_t width, height; /* Size (including title bar) */
  char title[32];        /* Window title */
  bool visible;          /* Is window visible? */
  bool focused;          /* Is window focused? */
  bool dragging;         /* Being dragged? */
  int32_t drag_offset_x; /* Offset from mouse to window corner */
  int32_t drag_offset_y;
  void (*draw_content)(int32_t x, int32_t y, int32_t w, int32_t h);
  void (*on_close)(void);
} window_t;

/* Window list */
static window_t windows[MAX_WINDOWS];
static int32_t window_count = 0;
static int32_t focused_window = -1;

/* Mouse state tracking */
static bool prev_mouse_left = false;

/* =============================================================================
 * SECTION 3: Window Management Functions
 * =============================================================================
 */

/*
 * window_init - Initialize the window manager
 */
void window_init(void) {
  window_count = 0;
  focused_window = -1;
  prev_mouse_left = false;

  for (int i = 0; i < MAX_WINDOWS; i++) {
    windows[i].visible = false;
  }

  debug_print("[WINDOW] Window manager initialized\n");
}

/*
 * window_create - Create a new window
 *
 * Returns: Window ID (index) or -1 if failed
 */
int32_t window_create(const char *title, int32_t x, int32_t y, int32_t width,
                      int32_t height) {
  if (window_count >= MAX_WINDOWS) {
    debug_print("[WINDOW] ERROR: Max windows reached\n");
    return -1;
  }

  int32_t id = window_count++;
  window_t *win = &windows[id];

  win->x = x;
  win->y = y;
  win->width = width;
  win->height = height + TITLE_BAR_HEIGHT;
  win->visible = true;
  win->focused = false;
  win->dragging = false;
  win->draw_content = NULL;
  win->on_close = NULL;

  /* Copy title */
  int i;
  for (i = 0; i < 31 && title[i]; i++) {
    win->title[i] = title[i];
  }
  win->title[i] = '\0';

  /* Focus new window */
  focused_window = id;
  win->focused = true;

  debug_print("[WINDOW] Created window: ");
  debug_print(title);
  debug_print("\n");

  return id;
}

/*
 * window_set_content_callback - Set function to draw window content
 */
void window_set_content_callback(int32_t id,
                                 void (*callback)(int32_t x, int32_t y,
                                                  int32_t w, int32_t h)) {
  if (id >= 0 && id < window_count) {
    windows[id].draw_content = callback;
  }
}

/*
 * window_set_close_callback - Set function called when window closes
 */
void window_set_close_callback(int32_t id, void (*callback)(void)) {
  if (id >= 0 && id < window_count) {
    windows[id].on_close = callback;
  }
}

/*
 * window_close - Close/hide a window
 */
void window_close(int32_t id) {
  if (id >= 0 && id < window_count) {
    window_t *win = &windows[id];
    if (win->on_close) {
      win->on_close();
    }
    win->visible = false;
    if (focused_window == id) {
      focused_window = -1;
    }
  }
}

/* =============================================================================
 * SECTION 4: Hit Testing
 * =============================================================================
 */

static bool point_in_rect(int32_t px, int32_t py, int32_t x, int32_t y,
                          int32_t w, int32_t h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}

static bool point_in_title_bar(window_t *win, int32_t mx, int32_t my) {
  return point_in_rect(mx, my, win->x, win->y, win->width - CLOSE_BTN_SIZE - 4,
                       TITLE_BAR_HEIGHT);
}

static bool point_in_close_button(window_t *win, int32_t mx, int32_t my) {
  int32_t bx = win->x + win->width - CLOSE_BTN_SIZE - 2;
  int32_t by = win->y + 2;
  return point_in_rect(mx, my, bx, by, CLOSE_BTN_SIZE, CLOSE_BTN_SIZE);
}

static bool point_in_window(window_t *win, int32_t mx, int32_t my) {
  return point_in_rect(mx, my, win->x, win->y, win->width, win->height);
}

/* =============================================================================
 * SECTION 5: Window Update (Input Handling)
 * =============================================================================
 */

/*
 * window_update - Process mouse input for windows
 *
 * Call this every frame to handle window dragging and focus.
 */
void window_update(void) {
  int32_t mx = mouse_get_x();
  int32_t my = mouse_get_y();
  bool mouse_left = mouse_is_left_pressed();
  bool mouse_clicked = mouse_left && !prev_mouse_left;
  bool mouse_released = !mouse_left && prev_mouse_left;

  prev_mouse_left = mouse_left;

  /* Handle dragging */
  for (int i = window_count - 1; i >= 0; i--) {
    window_t *win = &windows[i];
    if (!win->visible)
      continue;

    if (win->dragging) {
      if (mouse_left) {
        /* Update position while dragging */
        win->x = mx - win->drag_offset_x;
        win->y = my - win->drag_offset_y;

        /* Keep on screen */
        if (win->x < 0)
          win->x = 0;
        if (win->y < 0)
          win->y = 0;
        if (win->x + win->width > 320)
          win->x = 320 - win->width;
        if (win->y + win->height > 200)
          win->y = 200 - win->height;
      } else {
        win->dragging = false;
      }
      return;
    }
  }

  /* Handle clicks */
  if (mouse_clicked) {
    /* Check windows in reverse order (topmost first) */
    for (int i = window_count - 1; i >= 0; i--) {
      window_t *win = &windows[i];
      if (!win->visible)
        continue;

      if (point_in_window(win, mx, my)) {
        /* Focus this window */
        if (focused_window >= 0) {
          windows[focused_window].focused = false;
        }
        focused_window = i;
        win->focused = true;

        /* Check close button */
        if (point_in_close_button(win, mx, my)) {
          window_close(i);
          return;
        }

        /* Check title bar for drag */
        if (point_in_title_bar(win, mx, my)) {
          win->dragging = true;
          win->drag_offset_x = mx - win->x;
          win->drag_offset_y = my - win->y;
        }

        return; /* Consumed click */
      }
    }
  }
}

/* =============================================================================
 * SECTION 6: Window Rendering
 * =============================================================================
 */

/*
 * window_draw_single - Draw a single window
 */
static void window_draw_single(window_t *win) {
  if (!win->visible)
    return;

  int32_t x = win->x;
  int32_t y = win->y;
  int32_t w = win->width;
  int32_t h = win->height;

  /* Draw border */
  vga_draw_rect_outline(x, y, w, h, COLOR_BORDER);

  /* Draw title bar */
  uint8_t title_color = win->focused ? COLOR_TITLE_BAR : COLOR_BORDER;
  vga_draw_rect(x + 1, y + 1, w - 2, TITLE_BAR_HEIGHT - 1, title_color);

  /* Draw title text */
  font_draw_string(x + 4, y + 3, win->title, COLOR_TITLE_TEXT, title_color);

  /* Draw close button */
  int32_t bx = x + w - CLOSE_BTN_SIZE - 2;
  int32_t by = y + 2;
  vga_draw_rect(bx, by, CLOSE_BTN_SIZE, CLOSE_BTN_SIZE, COLOR_CLOSE_BTN);
  font_draw_string(bx + 2, by + 1, "X", COLOR_TITLE_TEXT, COLOR_CLOSE_BTN);

  /* Draw content area */
  int32_t cx = x + 1;
  int32_t cy = y + TITLE_BAR_HEIGHT;
  int32_t cw = w - 2;
  int32_t ch = h - TITLE_BAR_HEIGHT - 1;
  vga_draw_rect(cx, cy, cw, ch, COLOR_WINDOW_BG);

  /* Call content draw callback */
  if (win->draw_content) {
    win->draw_content(cx, cy, cw, ch);
  }
}

/*
 * window_draw_all - Draw all visible windows
 */
void window_draw_all(void) {
  /* Draw non-focused windows first */
  for (int i = 0; i < window_count; i++) {
    if (windows[i].visible && !windows[i].focused) {
      window_draw_single(&windows[i]);
    }
  }

  /* Draw focused window last (on top) */
  if (focused_window >= 0 && windows[focused_window].visible) {
    window_draw_single(&windows[focused_window]);
  }
}

/*
 * window_get_focused - Get the focused window ID
 */
int32_t window_get_focused(void) { return focused_window; }

/*
 * window_is_visible - Check if a window is visible
 */
bool window_is_visible(int32_t id) {
  if (id >= 0 && id < window_count) {
    return windows[id].visible;
  }
  return false;
}

/*
 * window_show - Show a hidden window
 */
void window_show(int32_t id) {
  if (id >= 0 && id < window_count) {
    windows[id].visible = true;
  }
}
