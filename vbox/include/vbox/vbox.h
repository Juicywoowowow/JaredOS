#ifndef VBOX_H
#define VBOX_H

#include "bios.h"
#include "cpu.h"
#include "memory.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Main Emulator Context
 *============================================================================*/

typedef struct VBoxDisplay VBoxDisplay;

typedef struct {
  VBoxCPU cpu;
  VBoxMemory *memory;
  VBoxBIOS bios;
  VBoxDisplay *display;

  /* Configuration */
  bool debug_mode;
  bool verbose;
  u32 memory_size_kb;

  /* Runtime state */
  bool running;
  u64 total_instructions;
} VBox;

/*============================================================================
 * Main API
 *============================================================================*/

/**
 * Create and initialize emulator
 * @param memory_kb Memory size in KB (default 640)
 */
VBox *vbox_create(u32 memory_kb);

/**
 * Destroy emulator and free all resources
 */
void vbox_destroy(VBox *vbox);

/**
 * Load binary file at specified address
 * @param vbox Emulator context
 * @param filename Path to .bin file
 * @param load_addr Linear address to load at (default 0x7C00 for boot sector)
 */
VBoxError vbox_load_binary(VBox *vbox, const char *filename, u32 load_addr);

/**
 * Run emulator until halt or error
 */
VBoxError vbox_run(VBox *vbox);

/**
 * Execute single instruction
 */
VBoxError vbox_step(VBox *vbox);

/**
 * Initialize display (SDL2 window)
 */
VBoxError vbox_init_display(VBox *vbox, const char *title, int scale);

/**
 * Update display from VGA memory
 */
void vbox_update_display(VBox *vbox);

#ifdef __cplusplus
}
#endif

#endif /* VBOX_H */
