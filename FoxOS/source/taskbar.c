/*
 * =============================================================================
 * taskbar.c - Taskbar for FoxOS
 * =============================================================================
 *
 * Implements a taskbar at the bottom of the screen with:
 *   - Start button (opens a simple menu placeholder)
 *   - Clock display showing uptime
 *   - Visual indicator area
 *
 * DEBUGGING TIPS:
 *   - If taskbar doesn't appear, check it's drawn after desktop clear
 *   - Timer must be running for clock to update
 *   - Taskbar is at y = 200 - TASKBAR_HEIGHT
 *
 * =============================================================================
 */

#include "types.h"

/* External functions */
extern void vga_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                          uint8_t color);
extern void vga_draw_button(int32_t x, int32_t y, int32_t w, int32_t h,
                            bool pressed);
extern void font_draw_string(int32_t x, int32_t y, const char *str, uint8_t fg,
                             uint8_t bg);
extern void font_draw_int(int32_t x, int32_t y, int32_t val, uint8_t fg,
                          uint8_t bg);
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern bool mouse_is_left_pressed(void);
extern uint32_t timer_get_seconds(void);

/* =============================================================================
 * SECTION 1: Taskbar Constants
 * =============================================================================
 */

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define TASKBAR_HEIGHT 18
#define TASKBAR_Y (SCREEN_HEIGHT - TASKBAR_HEIGHT)

#define START_BTN_WIDTH 40
#define START_BTN_HEIGHT 14
#define START_BTN_X 2
#define START_BTN_Y (TASKBAR_Y + 2)

#define CLOCK_WIDTH 50
#define CLOCK_X (SCREEN_WIDTH - CLOCK_WIDTH - 4)
#define CLOCK_Y (TASKBAR_Y + 5)

/* Colors */
#define COLOR_TASKBAR 19    /* Dark gray */
#define COLOR_START_TEXT 15 /* White */
#define COLOR_CLOCK_TEXT 15 /* White */

/* =============================================================================
 * SECTION 2: Taskbar State
 * =============================================================================
 */

static bool start_button_pressed = false;
static bool start_menu_open = false;
static bool prev_mouse_left = false;

/* =============================================================================
 * SECTION 3: Taskbar Initialization
 * =============================================================================
 */

void taskbar_init(void) {
  start_button_pressed = false;
  start_menu_open = false;
  prev_mouse_left = false;

  debug_print("[TASKBAR] Taskbar initialized\n");
}

/* =============================================================================
 * SECTION 4: Taskbar Update (Input)
 * =============================================================================
 */

static bool point_in_rect(int32_t px, int32_t py, int32_t x, int32_t y,
                          int32_t w, int32_t h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}

void taskbar_update(void) {
  int32_t mx = mouse_get_x();
  int32_t my = mouse_get_y();
  bool mouse_left = mouse_is_left_pressed();
  bool mouse_clicked = mouse_left && !prev_mouse_left;

  prev_mouse_left = mouse_left;

  /* Check start button hover/press */
  bool over_start = point_in_rect(mx, my, START_BTN_X, START_BTN_Y,
                                  START_BTN_WIDTH, START_BTN_HEIGHT);

  start_button_pressed = over_start && mouse_left;

  if (over_start && mouse_clicked) {
    start_menu_open = !start_menu_open;
    debug_print("[TASKBAR] Start menu toggled\n");
  }

  /* Close menu if clicked elsewhere */
  if (mouse_clicked && !over_start && start_menu_open) {
    /* Check if click is outside menu */
    if (!point_in_rect(mx, my, START_BTN_X, TASKBAR_Y - 80, 80, 80)) {
      start_menu_open = false;
    }
  }
}

/* =============================================================================
 * SECTION 5: Taskbar Rendering
 * =============================================================================
 */

void taskbar_draw(void) {
  /* Draw taskbar background */
  vga_draw_rect(0, TASKBAR_Y, SCREEN_WIDTH, TASKBAR_HEIGHT, COLOR_TASKBAR);

  /* Draw top border line (highlight) */
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    extern void vga_put_pixel(int32_t x, int32_t y, uint8_t color);
    vga_put_pixel(x, TASKBAR_Y, 21); /* Light color */
  }

  /* Draw start button */
  vga_draw_button(START_BTN_X, START_BTN_Y, START_BTN_WIDTH, START_BTN_HEIGHT,
                  start_button_pressed);

  int32_t text_offset = start_button_pressed ? 1 : 0;
  font_draw_string(START_BTN_X + 6 + text_offset, START_BTN_Y + 3 + text_offset,
                   "Start", COLOR_START_TEXT, 20); /* Button color */

  /* Draw clock */
  uint32_t secs = timer_get_seconds();
  uint32_t mins = secs / 60;
  uint32_t hours = mins / 60;
  secs %= 60;
  mins %= 60;

  /* Format as HH:MM:SS */
  char clock_str[9];
  clock_str[0] = '0' + (hours / 10) % 10;
  clock_str[1] = '0' + hours % 10;
  clock_str[2] = ':';
  clock_str[3] = '0' + mins / 10;
  clock_str[4] = '0' + mins % 10;
  clock_str[5] = ':';
  clock_str[6] = '0' + secs / 10;
  clock_str[7] = '0' + secs % 10;
  clock_str[8] = '\0';

  /* Draw clock background */
  vga_draw_rect(CLOCK_X - 2, TASKBAR_Y + 2, CLOCK_WIDTH + 4, 14, 22);

  /* Draw clock text */
  font_draw_string(CLOCK_X, CLOCK_Y, clock_str, COLOR_CLOCK_TEXT, 22);

  /* Draw start menu if open */
  if (start_menu_open) {
    int32_t menu_x = START_BTN_X;
    int32_t menu_y = TASKBAR_Y - 60;
    int32_t menu_w = 80;
    int32_t menu_h = 60;

    /* Menu background */
    vga_draw_rect(menu_x, menu_y, menu_w, menu_h, 17);
    extern void vga_draw_rect_outline(int32_t x, int32_t y, int32_t w,
                                      int32_t h, uint8_t c);
    vga_draw_rect_outline(menu_x, menu_y, menu_w, menu_h, 8);

    /* Menu items */
    font_draw_string(menu_x + 4, menu_y + 4, "FoxOS", 15, 17);
    font_draw_string(menu_x + 4, menu_y + 16, "--------", 8, 17);
    font_draw_string(menu_x + 4, menu_y + 28, "Pong", 0, 17);
    font_draw_string(menu_x + 4, menu_y + 40, "About", 0, 17);
  }
}

/*
 * taskbar_get_height - Get taskbar height (for other components)
 */
int32_t taskbar_get_height(void) { return TASKBAR_HEIGHT; }

/*
 * taskbar_get_y - Get taskbar Y position
 */
int32_t taskbar_get_y(void) { return TASKBAR_Y; }

/*
 * taskbar_is_menu_open - Check if start menu is open
 */
bool taskbar_is_menu_open(void) { return start_menu_open; }
