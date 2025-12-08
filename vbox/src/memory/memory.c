/*
 * VBox - Simple x86 Emulator
 * Memory Subsystem Implementation
 */

#include "vbox/memory.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Memory Lifecycle
 *============================================================================*/

VBoxMemory *mem_create(u32 size) {
  VBoxMemory *mem = (VBoxMemory *)malloc(sizeof(VBoxMemory));
  if (!mem)
    return NULL;

  /* Ensure size is at least 1MB and aligned */
  if (size < VBOX_MEMORY_SIZE) {
    size = VBOX_MEMORY_SIZE;
  }

  mem->data = (u8 *)calloc(size, 1);
  if (!mem->data) {
    free(mem);
    return NULL;
  }

  mem->size = size;
  memset(mem->readonly, 0, sizeof(mem->readonly));

  return mem;
}

void mem_destroy(VBoxMemory *mem) {
  if (mem) {
    free(mem->data);
    free(mem);
  }
}

void mem_clear(VBoxMemory *mem) {
  if (mem && mem->data) {
    memset(mem->data, 0, mem->size);
  }
}

/*============================================================================
 * Memory Read Operations
 *============================================================================*/

u8 mem_read8(VBoxMemory *mem, u32 addr) {
  addr &= (VBOX_MEMORY_SIZE - 1); /* Wrap to 1MB */
  return mem->data[addr];
}

u16 mem_read16(VBoxMemory *mem, u32 addr) {
  addr &= (VBOX_MEMORY_SIZE - 1);
  /* Little-endian: low byte first */
  return (u16)mem->data[addr] |
         ((u16)mem->data[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8);
}

u32 mem_read32(VBoxMemory *mem, u32 addr) {
  addr &= (VBOX_MEMORY_SIZE - 1);
  return (u32)mem->data[addr] |
         ((u32)mem->data[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] << 8) |
         ((u32)mem->data[(addr + 2) & (VBOX_MEMORY_SIZE - 1)] << 16) |
         ((u32)mem->data[(addr + 3) & (VBOX_MEMORY_SIZE - 1)] << 24);
}

/*============================================================================
 * Memory Write Operations
 *============================================================================*/

void mem_write8(VBoxMemory *mem, u32 addr, u8 value) {
  addr &= (VBOX_MEMORY_SIZE - 1);

  /* Check readonly flag (4KB pages) */
  if (mem->readonly[addr >> 12]) {
    return; /* Silently ignore writes to ROM */
  }

  mem->data[addr] = value;
}

void mem_write16(VBoxMemory *mem, u32 addr, u16 value) {
  addr &= (VBOX_MEMORY_SIZE - 1);

  /* Check readonly flag */
  if (mem->readonly[addr >> 12]) {
    return;
  }

  mem->data[addr] = (u8)(value & 0xFF);
  mem->data[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] = (u8)(value >> 8);
}

void mem_write32(VBoxMemory *mem, u32 addr, u32 value) {
  addr &= (VBOX_MEMORY_SIZE - 1);

  if (mem->readonly[addr >> 12]) {
    return;
  }

  mem->data[addr] = (u8)(value & 0xFF);
  mem->data[(addr + 1) & (VBOX_MEMORY_SIZE - 1)] = (u8)((value >> 8) & 0xFF);
  mem->data[(addr + 2) & (VBOX_MEMORY_SIZE - 1)] = (u8)((value >> 16) & 0xFF);
  mem->data[(addr + 3) & (VBOX_MEMORY_SIZE - 1)] = (u8)(value >> 24);
}

/*============================================================================
 * Block Operations
 *============================================================================*/

void mem_load(VBoxMemory *mem, u32 addr, const u8 *data, u32 size) {
  for (u32 i = 0; i < size; i++) {
    u32 target = (addr + i) & (VBOX_MEMORY_SIZE - 1);
    mem->data[target] = data[i];
  }
}

void mem_dump(VBoxMemory *mem, u32 addr, u8 *buffer, u32 size) {
  for (u32 i = 0; i < size; i++) {
    buffer[i] = mem->data[(addr + i) & (VBOX_MEMORY_SIZE - 1)];
  }
}
