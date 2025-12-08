#ifndef VBOX_MEMORY_H
#define VBOX_MEMORY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Memory Subsystem
 *============================================================================*/

/**
 * Memory context - wraps the raw memory array with metadata
 */
typedef struct {
  u8 *data;           /* Raw memory data (1MB) */
  u32 size;           /* Total allocated size */
  bool readonly[256]; /* 4KB page readonly flags (256 * 4KB = 1MB) */
} VBoxMemory;

/*============================================================================
 * Memory Functions
 *============================================================================*/

/**
 * Allocate and initialize memory
 * Returns NULL on failure
 */
VBoxMemory *mem_create(u32 size);

/**
 * Free memory
 */
void mem_destroy(VBoxMemory *mem);

/**
 * Zero all memory
 */
void mem_clear(VBoxMemory *mem);

/*============================================================================
 * Memory Access Functions
 *============================================================================*/

/**
 * Read byte from linear address
 */
u8 mem_read8(VBoxMemory *mem, u32 addr);

/**
 * Read word from linear address (little-endian)
 */
u16 mem_read16(VBoxMemory *mem, u32 addr);

/**
 * Read dword from linear address (little-endian)
 */
u32 mem_read32(VBoxMemory *mem, u32 addr);

/**
 * Write byte to linear address
 */
void mem_write8(VBoxMemory *mem, u32 addr, u8 value);

/**
 * Write word to linear address (little-endian)
 */
void mem_write16(VBoxMemory *mem, u32 addr, u16 value);

/**
 * Write dword to linear address (little-endian)
 */
void mem_write32(VBoxMemory *mem, u32 addr, u32 value);

/*============================================================================
 * Segment:Offset Access (Convenience)
 *============================================================================*/

/**
 * Read byte from segment:offset
 */
static inline u8 mem_read8_seg(VBoxMemory *mem, u16 seg, u16 offset) {
  return mem_read8(mem, ((u32)seg << 4) + offset);
}

/**
 * Read word from segment:offset
 */
static inline u16 mem_read16_seg(VBoxMemory *mem, u16 seg, u16 offset) {
  return mem_read16(mem, ((u32)seg << 4) + offset);
}

/**
 * Write byte to segment:offset
 */
static inline void mem_write8_seg(VBoxMemory *mem, u16 seg, u16 offset,
                                  u8 value) {
  mem_write8(mem, ((u32)seg << 4) + offset, value);
}

/**
 * Write word to segment:offset
 */
static inline void mem_write16_seg(VBoxMemory *mem, u16 seg, u16 offset,
                                   u16 value) {
  mem_write16(mem, ((u32)seg << 4) + offset, value);
}

/*============================================================================
 * Block Operations
 *============================================================================*/

/**
 * Copy data into memory from host buffer
 */
void mem_load(VBoxMemory *mem, u32 addr, const u8 *data, u32 size);

/**
 * Copy data from memory to host buffer
 */
void mem_dump(VBoxMemory *mem, u32 addr, u8 *buffer, u32 size);

/**
 * Get direct pointer to memory (for VGA, etc.)
 * Warning: No bounds checking!
 */
static inline u8 *mem_ptr(VBoxMemory *mem, u32 addr) {
  return &mem->data[addr & (VBOX_MEMORY_SIZE - 1)];
}

#ifdef __cplusplus
}
#endif

#endif /* VBOX_MEMORY_H */
