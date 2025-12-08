/*
 * VBox - Simple x86 Emulator
 * INT 13h - Disk BIOS Services (Stub)
 */

#include "vbox/bios.h"

/*============================================================================
 * INT 13h Handler
 *============================================================================*/

VBoxError bios_int13h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem) {
  (void)bios;
  (void)mem;

  u8 function = cpu->a.h;

  switch (function) {
  /*====================================================================
   * AH=00h: Reset disk system
   *====================================================================*/
  case 0x00:
    cpu->a.h = 0; /* Success */
    CPU_CLEAR_FLAG(cpu, FLAG_CF);
    break;

  /*====================================================================
   * AH=02h: Read sectors
   *====================================================================*/
  case 0x02:
    /* For now, just return success without actually reading */
    cpu->a.h = 0;
    cpu->a.l = cpu->a.l; /* Sectors read = sectors requested */
    CPU_CLEAR_FLAG(cpu, FLAG_CF);
    break;

  /*====================================================================
   * AH=03h: Write sectors
   *====================================================================*/
  case 0x03:
    cpu->a.h = 0;
    cpu->a.l = cpu->a.l;
    CPU_CLEAR_FLAG(cpu, FLAG_CF);
    break;

  /*====================================================================
   * AH=08h: Get drive parameters
   *====================================================================*/
  case 0x08:
    /* Return fake floppy parameters */
    cpu->a.h = 0;    /* Success */
    cpu->b.l = 0x04; /* Drive type (1.44MB floppy) */
    cpu->c.h = 79;   /* Max cylinder */
    cpu->c.l = 18;   /* Sectors per track */
    cpu->d.h = 1;    /* Max head */
    cpu->d.l = 1;    /* Number of drives */
    CPU_CLEAR_FLAG(cpu, FLAG_CF);
    break;

  default:
    /* Unsupported - return error */
    cpu->a.h = 0x01; /* Invalid function */
    CPU_SET_FLAG(cpu, FLAG_CF);
    break;
  }

  return VBOX_OK;
}
