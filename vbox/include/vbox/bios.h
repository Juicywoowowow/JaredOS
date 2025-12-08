#ifndef VBOX_BIOS_H
#define VBOX_BIOS_H

#include "cpu.h"
#include "memory.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Forward Declarations
 *============================================================================*/

/* Defined in display.hpp, implemented as extern "C" */
typedef struct VBoxDisplay VBoxDisplay;

/*============================================================================
 * BIOS Context
 *============================================================================*/

typedef struct {
  /* Video state */
  u8 video_mode;
  u8 cursor_x;
  u8 cursor_y;
  u8 cursor_start_line;
  u8 cursor_end_line;
  u8 active_page;
  u8 text_attribute;

  /* Keyboard state */
  u8 keyboard_buffer[16];
  u8 kbd_buf_head;
  u8 kbd_buf_tail;
  u8 shift_flags;

  /* Display reference */
  VBoxDisplay *display;
} VBoxBIOS;

/*============================================================================
 * BIOS Functions
 *============================================================================*/

/**
 * Initialize BIOS state
 */
void bios_init(VBoxBIOS *bios);

/**
 * Handle BIOS interrupt
 * Returns VBOX_OK or error code
 */
VBoxError bios_interrupt(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem,
                         u8 vector);

/*============================================================================
 * INT 10h - Video Services
 *============================================================================*/

VBoxError bios_int10h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem);

/*============================================================================
 * INT 13h - Disk Services
 *============================================================================*/

VBoxError bios_int13h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem);

/*============================================================================
 * INT 16h - Keyboard Services
 *============================================================================*/

VBoxError bios_int16h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem);

/*============================================================================
 * INT 21h - DOS Services (Minimal)
 *============================================================================*/

VBoxError bios_int21h(VBoxBIOS *bios, VBoxCPU *cpu, VBoxMemory *mem);

#ifdef __cplusplus
}
#endif

#endif /* VBOX_BIOS_H */
