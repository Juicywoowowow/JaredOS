/*
 * VBox - Simple x86 Emulator
 * INT 10h - Video BIOS Services
 */

#include "vbox/bios.h"
#include "vbox/display.hpp"

/*============================================================================
 * Video Service Handlers
 *============================================================================*/

/**
 * Write character to VGA text memory at cursor position
 */
static void write_char_at_cursor(VBoxBIOS *bios, VBoxMemory *mem, u8 ch,
                                 u8 attr) {
  u32 addr = VBOX_VGA_TEXT_BASE +
             (bios->cursor_y * VGA_TEXT_COLS + bios->cursor_x) * 2;
  mem_write8(mem, addr, ch);
  mem_write8(mem, addr + 1, attr);
}

/**
 * Advance cursor, handling line wrap and scroll
 */
static void advance_cursor(VBoxBIOS *bios, VBoxMemory *mem) {
  bios->cursor_x++;
  if (bios->cursor_x >= VGA_TEXT_COLS) {
    bios->cursor_x = 0;
    bios->cursor_y++;
    if (bios->cursor_y >= VGA_TEXT_ROWS) {
      /* Scroll up one line */
      bios->cursor_y = VGA_TEXT_ROWS - 1;

      /* Move lines 1-24 to 0-23 */
      for (int y = 0; y < VGA_TEXT_ROWS - 1; y++) {
        for (int x = 0; x < VGA_TEXT_COLS; x++) {
          u32 src = VBOX_VGA_TEXT_BASE + ((y + 1) * VGA_TEXT_COLS + x) * 2;
          u32 dst = VBOX_VGA_TEXT_BASE + (y * VGA_TEXT_COLS + x) * 2;
          mem_write8(mem, dst, mem_read8(mem, src));
          mem_write8(mem, dst + 1, mem_read8(mem, src + 1));
        }
      }

      /* Clear bottom line */
      for (int x = 0; x < VGA_TEXT_COLS; x++) {
        u32 addr =
            VBOX_VGA_TEXT_BASE + ((VGA_TEXT_ROWS - 1) * VGA_TEXT_COLS + x) * 2;
        mem_write8(mem, addr, ' ');
        mem_write8(mem, addr + 1, bios->text_attribute);
      }
    }
  }
}

/**
 * Clear screen
 */
static void clear_screen(VBoxBIOS *bios, VBoxMemory *mem) {
  for (int i = 0; i < VGA_TEXT_COLS * VGA_TEXT_ROWS; i++) {
    u32 addr = VBOX_VGA_TEXT_BASE + i * 2;
    mem_write8(mem, addr, ' ');
    mem_write8(mem, addr + 1, bios->text_attribute);
  }
  bios->cursor_x = 0;
  bios->cursor_y = 0;
}

/*============================================================================
 * INT 10h Handler
 *============================================================================*/

VBoxError bios_int10h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem) {
  u8 function = cpu->a.h;

  switch (function) {
  /*====================================================================
   * AH=00h: Set video mode
   *====================================================================*/
  case 0x00:
    bios->video_mode = cpu->a.l;
    clear_screen(bios, mem);
    break;

  /*====================================================================
   * AH=01h: Set cursor shape
   *====================================================================*/
  case 0x01:
    bios->cursor_start_line = cpu->c.h & 0x1F;
    bios->cursor_end_line = cpu->c.l & 0x1F;
    break;

  /*====================================================================
   * AH=02h: Set cursor position
   *====================================================================*/
  case 0x02:
    bios->cursor_y = cpu->d.h;
    bios->cursor_x = cpu->d.l;
    if (bios->cursor_x >= VGA_TEXT_COLS)
      bios->cursor_x = VGA_TEXT_COLS - 1;
    if (bios->cursor_y >= VGA_TEXT_ROWS)
      bios->cursor_y = VGA_TEXT_ROWS - 1;
    break;

  /*====================================================================
   * AH=03h: Get cursor position
   *====================================================================*/
  case 0x03:
    cpu->d.h = bios->cursor_y;
    cpu->d.l = bios->cursor_x;
    cpu->c.h = bios->cursor_start_line;
    cpu->c.l = bios->cursor_end_line;
    break;

  /*====================================================================
   * AH=05h: Select active display page
   *====================================================================*/
  case 0x05:
    bios->active_page = cpu->a.l;
    break;

  /*====================================================================
   * AH=06h: Scroll up window
   *====================================================================*/
  case 0x06: {
    u8 lines = cpu->a.l;
    u8 attr = cpu->b.h;
    u8 top = cpu->c.h;
    u8 left = cpu->c.l;
    u8 bottom = cpu->d.h;
    u8 right = cpu->d.l;

    if (lines == 0) {
      /* Clear entire window */
      for (int y = top; y <= bottom && y < VGA_TEXT_ROWS; y++) {
        for (int x = left; x <= right && x < VGA_TEXT_COLS; x++) {
          u32 addr = VBOX_VGA_TEXT_BASE + (y * VGA_TEXT_COLS + x) * 2;
          mem_write8(mem, addr, ' ');
          mem_write8(mem, addr + 1, attr);
        }
      }
    } else {
      /* Scroll up */
      for (int y = top; y <= bottom - lines && y < VGA_TEXT_ROWS; y++) {
        for (int x = left; x <= right && x < VGA_TEXT_COLS; x++) {
          u32 src = VBOX_VGA_TEXT_BASE + ((y + lines) * VGA_TEXT_COLS + x) * 2;
          u32 dst = VBOX_VGA_TEXT_BASE + (y * VGA_TEXT_COLS + x) * 2;
          mem_write8(mem, dst, mem_read8(mem, src));
          mem_write8(mem, dst + 1, mem_read8(mem, src + 1));
        }
      }
      /* Clear new lines at bottom */
      for (int y = bottom - lines + 1; y <= bottom && y < VGA_TEXT_ROWS; y++) {
        for (int x = left; x <= right && x < VGA_TEXT_COLS; x++) {
          u32 addr = VBOX_VGA_TEXT_BASE + (y * VGA_TEXT_COLS + x) * 2;
          mem_write8(mem, addr, ' ');
          mem_write8(mem, addr + 1, attr);
        }
      }
    }
    break;
  }

  /*====================================================================
   * AH=07h: Scroll down window
   *====================================================================*/
  case 0x07: {
    u8 lines = cpu->a.l;
    u8 attr = cpu->b.h;
    u8 top = cpu->c.h;
    u8 left = cpu->c.l;
    u8 bottom = cpu->d.h;
    u8 right = cpu->d.l;

    if (lines == 0) {
      /* Clear entire window */
      for (int y = top; y <= bottom && y < VGA_TEXT_ROWS; y++) {
        for (int x = left; x <= right && x < VGA_TEXT_COLS; x++) {
          u32 addr = VBOX_VGA_TEXT_BASE + (y * VGA_TEXT_COLS + x) * 2;
          mem_write8(mem, addr, ' ');
          mem_write8(mem, addr + 1, attr);
        }
      }
    }
    break;
  }

  /*====================================================================
   * AH=08h: Read character and attribute at cursor
   *====================================================================*/
  case 0x08: {
    u32 addr = VBOX_VGA_TEXT_BASE +
               (bios->cursor_y * VGA_TEXT_COLS + bios->cursor_x) * 2;
    cpu->a.l = mem_read8(mem, addr);     /* Character */
    cpu->a.h = mem_read8(mem, addr + 1); /* Attribute */
    break;
  }

  /*====================================================================
   * AH=09h: Write character and attribute at cursor
   *====================================================================*/
  case 0x09: {
    u8 ch = cpu->a.l;
    u8 attr = cpu->b.l;
    u16 count = cpu->c.x;

    for (u16 i = 0; i < count; i++) {
      write_char_at_cursor(bios, mem, ch, attr);
      /* Note: This function doesn't advance cursor per BIOS spec */
    }
    break;
  }

  /*====================================================================
   * AH=0Ah: Write character at cursor (no attribute change)
   *====================================================================*/
  case 0x0A: {
    u8 ch = cpu->a.l;
    u16 count = cpu->c.x;

    u32 addr = VBOX_VGA_TEXT_BASE +
               (bios->cursor_y * VGA_TEXT_COLS + bios->cursor_x) * 2;
    u8 attr = mem_read8(mem, addr + 1); /* Keep existing attribute */

    for (u16 i = 0; i < count; i++) {
      write_char_at_cursor(bios, mem, ch, attr);
    }
    break;
  }

  /*====================================================================
   * AH=0Eh: Teletype output (most common)
   *====================================================================*/
  case 0x0E: {
    u8 ch = cpu->a.l;

    switch (ch) {
    case 0x07: /* Bell - ignore */
      break;
    case 0x08: /* Backspace */
      if (bios->cursor_x > 0) {
        bios->cursor_x--;
      }
      break;
    case 0x09: /* Tab */
      bios->cursor_x = (bios->cursor_x + 8) & ~7;
      if (bios->cursor_x >= VGA_TEXT_COLS) {
        bios->cursor_x = 0;
        bios->cursor_y++;
      }
      break;
    case 0x0A: /* Line feed */
      bios->cursor_y++;
      if (bios->cursor_y >= VGA_TEXT_ROWS) {
        bios->cursor_y = VGA_TEXT_ROWS - 1;
        /* Scroll (simplified - just move cursor) */
      }
      break;
    case 0x0D: /* Carriage return */
      bios->cursor_x = 0;
      break;
    default:
      write_char_at_cursor(bios, mem, ch, bios->text_attribute);
      advance_cursor(bios, mem);
      break;
    }
    break;
  }

  /*====================================================================
   * AH=0Fh: Get video mode
   *====================================================================*/
  case 0x0F:
    cpu->a.l = bios->video_mode;
    cpu->a.h = VGA_TEXT_COLS;
    cpu->b.h = bios->active_page;
    break;

  default:
    /* Unhandled function - silently ignore */
    break;
  }

  return VBOX_OK;
}
