/*
 * =============================================================================
 * main.c - Main Kernel Loop for FoxOS
 * =============================================================================
 *
 * This file contains the main kernel loop that:
 *   1. Clears the screen with desktop background
 *   2. Updates all subsystems (input, windows, etc.)
 *   3. Renders everything
 *   4. Swaps display buffers
 *
 * The loop runs as fast as possible, but rendering is synced to timer.
 *
 * DEBUGGING TIPS:
 *   - If screen is frozen, the loop may have crashed
 *   - Use debug_print to trace which function fails
 *   - Check for infinite loops in update functions
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: External Function Declarations
 * =============================================================================
 */

/* VGA */
extern void vga_clear(uint8_t color);
extern void vga_swap_buffers(void);
extern void vga_put_pixel(int32_t x, int32_t y, uint8_t color);
extern void vga_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                          uint8_t color);
extern int32_t vga_get_width(void);
extern int32_t vga_get_height(void);

/* Window */
extern void window_update(void);
extern void window_draw_all(void);

/* Taskbar */
extern void taskbar_update(void);
extern void taskbar_draw(void);
extern int32_t taskbar_get_y(void);

/* Pong */
extern void pong_update(void);
extern void pong_draw_content(int32_t x, int32_t y, int32_t w, int32_t h);
extern int32_t pong_get_window_id(void);
extern void window_set_content_callback(int32_t id,
                                        void (*cb)(int32_t, int32_t, int32_t,
                                                   int32_t));

/* Mouse */
extern int32_t mouse_get_x(void);
extern int32_t mouse_get_y(void);
extern bool mouse_is_left_pressed(void);

/* Timer */
extern uint32_t timer_get_ticks(void);

/* =============================================================================
 * SECTION 2: Desktop Constants
 * =============================================================================
 */

#define COLOR_DESKTOP 16 /* Nice blue */

/* =============================================================================
 * SECTION 3: Mouse Cursor
 * =============================================================================
 */

/* Simple arrow cursor (8x8) */
static const uint8_t cursor_data[8] = {
    0b10000000, 0b11000000, 0b11100000, 0b11110000,
    0b11111000, 0b11100000, 0b10100000, 0b00100000,
};

/*
 * draw_cursor - Draw the mouse cursor
 */
static void draw_cursor(int32_t x, int32_t y) {
  for (int32_t row = 0; row < 8; row++) {
    uint8_t bits = cursor_data[row];
    for (int32_t col = 0; col < 8; col++) {
      if (bits & (0x80 >> col)) {
        vga_put_pixel(x + col, y + row, 15); /* White */
      }
    }
  }
}

/* =============================================================================
 * SECTION 4: Main Kernel Loop
 * =============================================================================
 */

/*
 * kernel_main_loop - The heart of FoxOS
 *
 * This runs forever, processing input and rendering the display.
 */
void kernel_main_loop(void) {
  uint32_t last_frame_tick = 0;
  uint32_t frame_count = 0;

  /* Setup pong window content callback */
  int32_t pong_id = pong_get_window_id();
  if (pong_id >= 0) {
    window_set_content_callback(pong_id, pong_draw_content);
  }

  debug_print("[MAIN] Entering main loop\n");

  while (1) {
    /* Throttle to ~30 FPS (every 3 ticks at 100Hz) */
    uint32_t current_tick = timer_get_ticks();
    if (current_tick - last_frame_tick < 3) {
      hlt(); /* Wait for next interrupt */
      continue;
    }
    last_frame_tick = current_tick;
    frame_count++;

    /* ===== UPDATE PHASE ===== */

    /* Update game logic */
    pong_update();

    /* Update window manager (handle dragging, focus) */
    window_update();

    /* Update taskbar (handle start button) */
    taskbar_update();

    /* ===== RENDER PHASE ===== */

    /* Clear screen with desktop color */
    int32_t taskbar_y = taskbar_get_y();
    vga_draw_rect(0, 0, vga_get_width(), taskbar_y, COLOR_DESKTOP);

    /* Draw windows */
    window_draw_all();

    /* Draw taskbar (on top of everything except cursor) */
    taskbar_draw();

    /* Draw mouse cursor (always on top) */
    draw_cursor(mouse_get_x(), mouse_get_y());

    /* Swap buffers to display */
    vga_swap_buffers();

/* Debug: print FPS every 5 seconds */
#if 0
        if (frame_count % 150 == 0) {  /* ~5 sec at 30 FPS */
            debug_print("[MAIN] Frames: ");
            debug_hex(frame_count);
            debug_print("\n");
        }
#endif
  }
}
