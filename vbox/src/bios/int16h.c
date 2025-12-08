/*
 * VBox - Simple x86 Emulator
 * INT 16h - Keyboard BIOS Services
 */

#include "vbox/bios.h"

/*============================================================================
 * Keyboard Buffer Helpers
 *============================================================================*/

static bool kbd_buffer_empty(VBoxBIOS *bios) {
  return bios->kbd_buf_head == bios->kbd_buf_tail;
}

static void kbd_buffer_put(VBoxBIOS *bios, u8 scancode, u8 ascii) {
  u8 next_tail = (bios->kbd_buf_tail + 2) % 16;
  if (next_tail != bios->kbd_buf_head) {
    bios->keyboard_buffer[bios->kbd_buf_tail] = ascii;
    bios->keyboard_buffer[bios->kbd_buf_tail + 1] = scancode;
    bios->kbd_buf_tail = next_tail;
  }
}

static u16 kbd_buffer_get(VBoxBIOS *bios) {
  if (kbd_buffer_empty(bios)) {
    return 0;
  }
  u8 ascii = bios->keyboard_buffer[bios->kbd_buf_head];
  u8 scancode = bios->keyboard_buffer[bios->kbd_buf_head + 1];
  bios->kbd_buf_head = (bios->kbd_buf_head + 2) % 16;
  return ((u16)scancode << 8) | ascii;
}

static u16 kbd_buffer_peek(VBoxBIOS *bios) {
  if (kbd_buffer_empty(bios)) {
    return 0;
  }
  u8 ascii = bios->keyboard_buffer[bios->kbd_buf_head];
  u8 scancode = bios->keyboard_buffer[bios->kbd_buf_head + 1];
  return ((u16)scancode << 8) | ascii;
}

/*============================================================================
 * INT 16h Handler
 *============================================================================*/

VBoxError bios_int16h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem) {
  (void)mem; /* Unused */

  u8 function = cpu->a.h;

  switch (function) {
  /*====================================================================
   * AH=00h: Read keyboard character (blocking)
   *====================================================================*/
  case 0x00: {
    /* In a real emulator, this would block until a key is pressed.
     * For now, we return 0 if buffer is empty (non-blocking behavior).
     * The main loop should poll SDL events and fill the buffer. */
    u16 key = kbd_buffer_get(bios);
    cpu->a.x = key; /* AL=ASCII, AH=scancode */
    break;
  }

  /*====================================================================
   * AH=01h: Check for keystroke (non-blocking)
   *====================================================================*/
  case 0x01:
    if (kbd_buffer_empty(bios)) {
      CPU_SET_FLAG(cpu, FLAG_ZF); /* ZF=1 if no key available */
      cpu->a.x = 0;
    } else {
      CPU_CLEAR_FLAG(cpu, FLAG_ZF);
      cpu->a.x = kbd_buffer_peek(bios); /* Don't remove from buffer */
    }
    break;

  /*====================================================================
   * AH=02h: Get shift flags
   *====================================================================*/
  case 0x02:
    cpu->a.l = bios->shift_flags;
    break;

  /*====================================================================
   * AH=10h: Extended read (same as 00h for our purposes)
   *====================================================================*/
  case 0x10: {
    u16 key = kbd_buffer_get(bios);
    cpu->a.x = key;
    break;
  }

  /*====================================================================
   * AH=11h: Extended check (same as 01h for our purposes)
   *====================================================================*/
  case 0x11:
    if (kbd_buffer_empty(bios)) {
      CPU_SET_FLAG(cpu, FLAG_ZF);
      cpu->a.x = 0;
    } else {
      CPU_CLEAR_FLAG(cpu, FLAG_ZF);
      cpu->a.x = kbd_buffer_peek(bios);
    }
    break;

  /*====================================================================
   * AH=12h: Extended shift flags
   *====================================================================*/
  case 0x12:
    cpu->a.l = bios->shift_flags;
    cpu->a.h = 0; /* Extended flags (simplified) */
    break;

  default:
    break;
  }

  return VBOX_OK;
}

/*============================================================================
 * External Interface for Display/Input Handler
 *============================================================================*/

/**
 * Add a key to the BIOS keyboard buffer (called from SDL event handler)
 */
void bios_keyboard_inject(VBoxBIOS *bios, u8 scancode, u8 ascii) {
  kbd_buffer_put(bios, scancode, ascii);
}

/**
 * Update shift flags (called from SDL event handler)
 */
void bios_keyboard_set_shift(VBoxBIOS *bios, u8 flags) {
  bios->shift_flags = flags;
}
