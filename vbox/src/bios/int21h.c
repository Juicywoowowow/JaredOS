/*
 * VBox - Simple x86 Emulator
 * INT 21h - DOS Services (Minimal)
 */

#include "vbox/bios.h"
#include <stdio.h>

/*============================================================================
 * INT 21h Handler
 *============================================================================*/

VBoxError bios_int21h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem) {
  u8 function = cpu->a.h;

  switch (function) {
  /*====================================================================
   * AH=01h: Read character with echo
   *====================================================================*/
  case 0x01: {
    /* Use BIOS keyboard service */
    VBoxError err = bios_int16h(bios, cpu, mem);
    /* Echo character using INT 10h */
    u8 ch = cpu->a.l;
    u8 saved_ah = cpu->a.h;
    cpu->a.h = 0x0E;
    bios_int10h(bios, cpu, mem);
    cpu->a.h = saved_ah;
    cpu->a.l = ch;
    return err;
  }

  /*====================================================================
   * AH=02h: Print character
   *====================================================================*/
  case 0x02:
    cpu->a.l = cpu->d.l; /* Character to print */
    cpu->a.h = 0x0E;
    return bios_int10h(bios, cpu, mem);

  /*====================================================================
   * AH=09h: Print string ($ terminated)
   *====================================================================*/
  case 0x09: {
    u16 seg = cpu->ds;
    u16 offset = cpu->d.x;

    while (1) {
      u8 ch = mem_read8_seg(mem, seg, offset);
      if (ch == '$')
        break;

      /* Print using INT 10h teletype */
      cpu->a.l = ch;
      cpu->a.h = 0x0E;
      bios_int10h(bios, cpu, mem);

      offset++;
    }
    cpu->a.l = '$';
    break;
  }

  /*====================================================================
   * AH=4Ch: Exit program
   *====================================================================*/
  case 0x4C:
    cpu->halted = true;
    return VBOX_ERR_HALT;

  default:
    /* Unhandled - just return */
    break;
  }

  return VBOX_OK;
}
