/*
 * VBox - Simple x86 Emulator
 * BIOS Interrupt Dispatcher
 */

#include "vbox/bios.h"
#include "vbox/display.hpp"
#include <string.h>

/*============================================================================
 * BIOS Initialization
 *============================================================================*/

void bios_init(VBoxBIOS *bios) {
  memset(bios, 0, sizeof(VBoxBIOS));

  /* Default video mode: 80x25 text */
  bios->video_mode = 0x03;
  bios->cursor_x = 0;
  bios->cursor_y = 0;
  bios->cursor_start_line = 6;
  bios->cursor_end_line = 7;
  bios->active_page = 0;
  bios->text_attribute = 0x07; /* Light gray on black */

  /* Keyboard buffer is empty */
  bios->kbd_buf_head = 0;
  bios->kbd_buf_tail = 0;
  bios->shift_flags = 0;

  bios->display = NULL;
}

/*============================================================================
 * BIOS Interrupt Dispatcher
 *============================================================================*/

VBoxError bios_interrupt(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem,
                         u8 vector) {
  switch (vector) {
  case 0x10:
    return bios_int10h(bios, cpu, mem);

  case 0x13:
    return bios_int13h(bios, cpu, mem);

  case 0x16:
    return bios_int16h(bios, cpu, mem);

  case 0x21:
    return bios_int21h(bios, cpu, mem);

  default:
    /* Unhandled interrupt - just return */
    return VBOX_OK;
  }
}
