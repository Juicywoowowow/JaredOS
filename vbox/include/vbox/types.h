#ifndef VBOX_TYPES_H
#define VBOX_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Basic Type Definitions
 *============================================================================*/

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/*============================================================================
 * x86 Real Mode Constants
 *============================================================================*/

/* Memory size constants */
#define VBOX_MEMORY_SIZE (1024 * 1024)     /* 1MB for real mode */
#define VBOX_CONVENTIONAL_MEM (640 * 1024) /* 640KB conventional memory */

/* Video memory addresses */
#define VBOX_VGA_TEXT_BASE 0xB8000
#define VBOX_VGA_TEXT_SIZE (80 * 25 * 2) /* 80x25 characters, 2 bytes each */
#define VBOX_VGA_GRAPHICS_BASE 0xA0000
#define VBOX_VGA_GRAPHICS_SIZE (64 * 1024) /* 64KB graphics memory */

/* BIOS memory areas */
#define VBOX_BIOS_DATA_AREA 0x00400
#define VBOX_BIOS_ROM_START 0xF0000
#define VBOX_IVT_BASE 0x00000   /* Interrupt Vector Table */
#define VBOX_IVT_SIZE (256 * 4) /* 256 vectors, 4 bytes each */

/* Default segment values */
#define VBOX_DEFAULT_CS 0x0000
#define VBOX_DEFAULT_DS 0x0000
#define VBOX_DEFAULT_SS 0x0000
#define VBOX_DEFAULT_ES 0x0000

/* Stack defaults */
#define VBOX_DEFAULT_SP 0xFFFE
#define VBOX_DEFAULT_BP 0x0000

/*============================================================================
 * VGA Constants
 *============================================================================*/

#define VGA_TEXT_COLS 80
#define VGA_TEXT_ROWS 25
#define VGA_PIXEL_WIDTH 640
#define VGA_PIXEL_HEIGHT 400

/*============================================================================
 * Error Codes
 *============================================================================*/

typedef enum {
  VBOX_OK = 0,
  VBOX_ERR_MEMORY,
  VBOX_ERR_INVALID_OPCODE,
  VBOX_ERR_DIVIDE_BY_ZERO,
  VBOX_ERR_INVALID_INTERRUPT,
  VBOX_ERR_FILE_NOT_FOUND,
  VBOX_ERR_FILE_TOO_LARGE,
  VBOX_ERR_SDL_INIT,
  VBOX_ERR_HALT,
} VBoxError;

/*============================================================================
 * CPU State Flags (EFLAGS bits)
 *============================================================================*/

#define FLAG_CF (1 << 0)  /* Carry Flag */
#define FLAG_PF (1 << 2)  /* Parity Flag */
#define FLAG_AF (1 << 4)  /* Auxiliary Carry Flag */
#define FLAG_ZF (1 << 6)  /* Zero Flag */
#define FLAG_SF (1 << 7)  /* Sign Flag */
#define FLAG_TF (1 << 8)  /* Trap Flag */
#define FLAG_IF (1 << 9)  /* Interrupt Enable Flag */
#define FLAG_DF (1 << 10) /* Direction Flag */
#define FLAG_OF (1 << 11) /* Overflow Flag */

/*============================================================================
 * Instruction Prefixes
 *============================================================================*/

#define PREFIX_LOCK 0xF0
#define PREFIX_REPNE 0xF2
#define PREFIX_REP 0xF3
#define PREFIX_ES 0x26
#define PREFIX_CS 0x2E
#define PREFIX_SS 0x36
#define PREFIX_DS 0x3E
#define PREFIX_OPSIZE 0x66
#define PREFIX_ADDRSIZE 0x67

#ifdef __cplusplus
}
#endif

#endif /* VBOX_TYPES_H */
