/*
 * =============================================================================
 * vga.c - VGA Graphics Driver for FoxOS
 * =============================================================================
 *
 * This driver provides graphics functionality using VGA Mode 13h:
 *   - Resolution: 320x200 pixels
 *   - Colors: 256 (8-bit palette)
 *   - Linear framebuffer at 0xA0000
 *
 * Mode 13h is simple and perfect for hobby OS development:
 *   - No bank switching needed
 *   - Each byte in video memory = one pixel
 *   - Easy to understand and implement
 *
 * Double buffering is used to prevent screen flicker:
 *   1. Draw everything to back buffer (in RAM)
 *   2. Copy back buffer to video memory in one operation
 *
 * DEBUGGING TIPS:
 *   - If screen is black, VGA mode switch may have failed
 *   - If colors are wrong, check palette setup
 *   - If display flickers, make sure you're using double buffering
 *   - Port 0x3C8/0x3C9 control the color palette
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: VGA Constants and Memory
 * =============================================================================
 */

/* VGA Mode 13h parameters */
#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_BPP 1 /* Bytes per pixel */
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT)

/* VGA video memory address */
#define VGA_MEMORY 0xA0000

/* VGA register ports */
#define VGA_MISC_WRITE 0x3C2    /* Miscellaneous output register */
#define VGA_SEQ_INDEX 0x3C4     /* Sequencer index */
#define VGA_SEQ_DATA 0x3C5      /* Sequencer data */
#define VGA_PALETTE_INDEX 0x3C8 /* Palette index (write) */
#define VGA_PALETTE_DATA 0x3C9  /* Palette RGB data */
#define VGA_CRTC_INDEX 0x3D4    /* CRTC index */
#define VGA_CRTC_DATA 0x3D5     /* CRTC data */

/* Double buffer in RAM */
static uint8_t back_buffer[VGA_SIZE];

/* Pointer to video memory */
static uint8_t *const video_memory = (uint8_t *)VGA_MEMORY;

/* =============================================================================
 * SECTION 2: Standard Color Palette (VGA 256 colors)
 * =============================================================================
 * We'll set up a nice color palette for our GUI.
 */

/* Color indices (for convenience) */
#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHT_GRAY 7
#define COLOR_DARK_GRAY 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW 14
#define COLOR_WHITE 15

/* Additional GUI colors */
#define COLOR_DESKTOP 16      /* Desktop background */
#define COLOR_WINDOW_BG 17    /* Window background */
#define COLOR_WINDOW_TITLE 18 /* Window title bar */
#define COLOR_TASKBAR 19      /* Taskbar background */
#define COLOR_BUTTON 20       /* Button face */
#define COLOR_BUTTON_LIGHT 21 /* Button highlight */
#define COLOR_BUTTON_DARK 22  /* Button shadow */

/* Default palette RGB values (6-bit per channel, 0-63) */
static const uint8_t default_palette[][3] = {
    {0, 0, 0},    /* 0:  Black */
    {0, 0, 42},   /* 1:  Blue */
    {0, 42, 0},   /* 2:  Green */
    {0, 42, 42},  /* 3:  Cyan */
    {42, 0, 0},   /* 4:  Red */
    {42, 0, 42},  /* 5:  Magenta */
    {42, 21, 0},  /* 6:  Brown */
    {42, 42, 42}, /* 7:  Light Gray */
    {21, 21, 21}, /* 8:  Dark Gray */
    {21, 21, 63}, /* 9:  Light Blue */
    {21, 63, 21}, /* 10: Light Green */
    {21, 63, 63}, /* 11: Light Cyan */
    {63, 21, 21}, /* 12: Light Red */
    {63, 21, 63}, /* 13: Light Magenta */
    {63, 63, 21}, /* 14: Yellow */
    {63, 63, 63}, /* 15: White */
    {0, 32, 48},  /* 16: Desktop (nice blue) */
    {50, 50, 50}, /* 17: Window background */
    {16, 32, 48}, /* 18: Window title bar */
    {32, 32, 32}, /* 19: Taskbar */
    {40, 40, 40}, /* 20: Button face */
    {50, 50, 50}, /* 21: Button light */
    {20, 20, 20}, /* 22: Button dark */
};

/*
 * vga_set_palette_color - Set a single palette entry
 *
 * @param index: Palette index (0-255)
 * @param r, g, b: Color components (0-63)
 */
void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
  outb(VGA_PALETTE_INDEX, index);
  outb(VGA_PALETTE_DATA, r);
  outb(VGA_PALETTE_DATA, g);
  outb(VGA_PALETTE_DATA, b);
}

/*
 * vga_init_palette - Initialize the color palette
 */
static void vga_init_palette(void) {
  /* Set our custom palette entries */
  for (uint32_t i = 0; i < sizeof(default_palette) / 3; i++) {
    vga_set_palette_color(i, default_palette[i][0], default_palette[i][1],
                          default_palette[i][2]);
  }

  /* Fill rest of palette with a gradient (for potential use) */
  for (uint32_t i = 23; i < 256; i++) {
    /* Create a smooth gradient */
    uint8_t r = (i * 63) / 255;
    uint8_t g = ((i * 2) * 63) / 510;
    uint8_t b = ((255 - i) * 63) / 255;
    vga_set_palette_color(i, r, g, b);
  }

  debug_print("[VGA] Palette initialized\n");
}

/* =============================================================================
 * SECTION 3: VGA Mode Setting
 * =============================================================================
 */

/*
 * vga_set_mode_13h - Switch to VGA Mode 13h (320x200, 256 colors)
 *
 * We use BIOS interrupt 0x10 would work in real mode, but since we're
 * in protected mode, we write directly to VGA registers.
 *
 * For simplicity, we'll assume the BIOS already set mode 13h
 * (via bootloader or we can add BIOS call in real mode before switch).
 */
static void vga_set_mode_13h(void) {
  /*
   * In a real implementation, you'd either:
   * 1. Set mode 13h from bootloader before entering protected mode
   * 2. Use V86 mode to call BIOS
   * 3. Program all VGA registers directly (complex)
   *
   * For this demo, we assume mode 13h is already set.
   * Add this to boot.asm before protected mode switch:
   *   mov ax, 0x13
   *   int 0x10
   */

  debug_print("[VGA] Mode 13h (320x200x256) assumed active\n");
}

/* =============================================================================
 * SECTION 4: VGA Initialization
 * =============================================================================
 */

/*
 * vga_init - Initialize VGA graphics
 */
void vga_init(void) {
  debug_print("[VGA] Initializing VGA graphics...\n");

  /* Set video mode */
  vga_set_mode_13h();

  /* Initialize color palette */
  vga_init_palette();

  /* Clear both buffers */
  memset(back_buffer, COLOR_BLACK, VGA_SIZE);
  memset(video_memory, COLOR_BLACK, VGA_SIZE);

  debug_print("[VGA] Resolution: 320x200, 256 colors\n");
  debug_print("[VGA] VGA initialized successfully\n");
}

/* =============================================================================
 * SECTION 5: Buffer Operations
 * =============================================================================
 */

/*
 * vga_swap_buffers - Copy back buffer to video memory
 *
 * Call this after drawing a frame to display it.
 * This is where double-buffering prevents flicker.
 */
void vga_swap_buffers(void) { memcpy(video_memory, back_buffer, VGA_SIZE); }

/*
 * vga_clear - Clear the back buffer
 *
 * @param color: Color index to fill with
 */
void vga_clear(uint8_t color) { memset(back_buffer, color, VGA_SIZE); }

/*
 * vga_get_buffer - Get pointer to back buffer
 *
 * Returns: Pointer to the back buffer for direct manipulation
 */
uint8_t *vga_get_buffer(void) { return back_buffer; }

/* =============================================================================
 * SECTION 6: Pixel Drawing
 * =============================================================================
 */

/*
 * vga_put_pixel - Draw a single pixel to back buffer
 *
 * @param x, y: Pixel coordinates
 * @param color: Color index (0-255)
 */
void vga_put_pixel(int32_t x, int32_t y, uint8_t color) {
  /* Bounds checking */
  if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
    return;
  }

  back_buffer[y * VGA_WIDTH + x] = color;
}

/*
 * vga_get_pixel - Read a pixel from back buffer
 *
 * @param x, y: Pixel coordinates
 * Returns: Color index at that position
 */
uint8_t vga_get_pixel(int32_t x, int32_t y) {
  if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) {
    return 0;
  }

  return back_buffer[y * VGA_WIDTH + x];
}

/* =============================================================================
 * SECTION 7: Shape Drawing
 * =============================================================================
 */

/*
 * vga_draw_rect - Draw a filled rectangle
 *
 * @param x, y: Top-left corner
 * @param width, height: Dimensions
 * @param color: Fill color
 */
void vga_draw_rect(int32_t x, int32_t y, int32_t width, int32_t height,
                   uint8_t color) {
  /* Clip to screen bounds */
  int32_t x1 = MAX(x, 0);
  int32_t y1 = MAX(y, 0);
  int32_t x2 = MIN(x + width, VGA_WIDTH);
  int32_t y2 = MIN(y + height, VGA_HEIGHT);

  for (int32_t py = y1; py < y2; py++) {
    for (int32_t px = x1; px < x2; px++) {
      back_buffer[py * VGA_WIDTH + px] = color;
    }
  }
}

/*
 * vga_draw_rect_outline - Draw a rectangle outline
 *
 * @param x, y: Top-left corner
 * @param width, height: Dimensions
 * @param color: Line color
 */
void vga_draw_rect_outline(int32_t x, int32_t y, int32_t width, int32_t height,
                           uint8_t color) {
  /* Top and bottom lines */
  for (int32_t px = x; px < x + width; px++) {
    vga_put_pixel(px, y, color);
    vga_put_pixel(px, y + height - 1, color);
  }

  /* Left and right lines */
  for (int32_t py = y; py < y + height; py++) {
    vga_put_pixel(x, py, color);
    vga_put_pixel(x + width - 1, py, color);
  }
}

/*
 * vga_draw_line - Draw a line using Bresenham's algorithm
 *
 * @param x0, y0: Start point
 * @param x1, y1: End point
 * @param color: Line color
 */
void vga_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   uint8_t color) {
  int32_t dx = ABS(x1 - x0);
  int32_t dy = ABS(y1 - y0);
  int32_t sx = (x0 < x1) ? 1 : -1;
  int32_t sy = (y0 < y1) ? 1 : -1;
  int32_t err = dx - dy;

  while (1) {
    vga_put_pixel(x0, y0, color);

    if (x0 == x1 && y0 == y1)
      break;

    int32_t e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

/*
 * vga_draw_circle - Draw a filled circle
 *
 * @param cx, cy: Center coordinates
 * @param radius: Circle radius
 * @param color: Fill color
 */
void vga_draw_circle(int32_t cx, int32_t cy, int32_t radius, uint8_t color) {
  for (int32_t y = -radius; y <= radius; y++) {
    for (int32_t x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        vga_put_pixel(cx + x, cy + y, color);
      }
    }
  }
}

/*
 * vga_draw_circle_outline - Draw a circle outline
 *
 * @param cx, cy: Center coordinates
 * @param radius: Circle radius
 * @param color: Line color
 */
void vga_draw_circle_outline(int32_t cx, int32_t cy, int32_t radius,
                             uint8_t color) {
  int32_t x = radius;
  int32_t y = 0;
  int32_t err = 0;

  while (x >= y) {
    vga_put_pixel(cx + x, cy + y, color);
    vga_put_pixel(cx + y, cy + x, color);
    vga_put_pixel(cx - y, cy + x, color);
    vga_put_pixel(cx - x, cy + y, color);
    vga_put_pixel(cx - x, cy - y, color);
    vga_put_pixel(cx - y, cy - x, color);
    vga_put_pixel(cx + y, cy - x, color);
    vga_put_pixel(cx + x, cy - y, color);

    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    }
    if (err > 0) {
      x--;
      err -= 2 * x + 1;
    }
  }
}

/* =============================================================================
 * SECTION 8: 3D-style Button Drawing
 * =============================================================================
 */

/*
 * vga_draw_button - Draw a 3D-style button
 *
 * @param x, y: Position
 * @param width, height: Dimensions
 * @param pressed: true for pressed state
 */
void vga_draw_button(int32_t x, int32_t y, int32_t width, int32_t height,
                     bool pressed) {
  uint8_t face = COLOR_BUTTON;
  uint8_t light = pressed ? COLOR_BUTTON_DARK : COLOR_BUTTON_LIGHT;
  uint8_t shadow = pressed ? COLOR_BUTTON_LIGHT : COLOR_BUTTON_DARK;

  /* Fill button face */
  vga_draw_rect(x, y, width, height, face);

  /* Draw highlight (top and left edges) */
  for (int32_t px = x; px < x + width; px++) {
    vga_put_pixel(px, y, light);
  }
  for (int32_t py = y; py < y + height; py++) {
    vga_put_pixel(x, py, light);
  }

  /* Draw shadow (bottom and right edges) */
  for (int32_t px = x; px < x + width; px++) {
    vga_put_pixel(px, y + height - 1, shadow);
  }
  for (int32_t py = y; py < y + height; py++) {
    vga_put_pixel(x + width - 1, py, shadow);
  }
}

/* =============================================================================
 * SECTION 9: Utility Functions
 * =============================================================================
 */

/*
 * vga_get_width - Get screen width
 */
int32_t vga_get_width(void) { return VGA_WIDTH; }

/*
 * vga_get_height - Get screen height
 */
int32_t vga_get_height(void) { return VGA_HEIGHT; }
