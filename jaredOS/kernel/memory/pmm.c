/**
 * jaredOS - Physical Memory Manager Implementation
 */

#include "pmm.h"
#include "../lib/string.h"

/* Bitmap for tracking page frames */
static uint32_t *frame_bitmap = NULL;
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;
static uint32_t bitmap_size = 0;
static uint32_t mem_size_kb = 0;

/* Bitmap start address (placed after kernel) */
#define BITMAP_START 0x200000   /* 2MB - after kernel */

/**
 * Set a bit in the bitmap
 */
static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t offset = frame % 32;
    frame_bitmap[idx] |= (1 << offset);
}

/**
 * Clear a bit in the bitmap
 */
static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t offset = frame % 32;
    frame_bitmap[idx] &= ~(1 << offset);
}

/**
 * Test if a frame is used
 */
static int test_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t offset = frame % 32;
    return (frame_bitmap[idx] & (1 << offset)) ? 1 : 0;
}

/**
 * Find first free frame
 */
static uint32_t first_free_frame(void) {
    for (uint32_t i = 0; i < bitmap_size / 4; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(frame_bitmap[i] & (1 << j))) {
                    return (i * 32 + j) * PAGE_SIZE;
                }
            }
        }
    }
    return 0; /* No free frames */
}

/**
 * Initialize physical memory manager
 */
void pmm_init(uint32_t mem_kb) {
    mem_size_kb = mem_kb;
    total_frames = (mem_kb * 1024) / PAGE_SIZE;
    bitmap_size = total_frames / 8;
    if (bitmap_size % 4) bitmap_size += 4 - (bitmap_size % 4);

    frame_bitmap = (uint32_t*)BITMAP_START;

    /* Clear bitmap - all frames free */
    memset(frame_bitmap, 0, bitmap_size);

    /* Mark kernel area as used (0 - 2MB) */
    for (uint32_t i = 0; i < (BITMAP_START + bitmap_size); i += PAGE_SIZE) {
        set_frame(i);
        used_frames++;
    }
}

/**
 * Allocate a physical frame
 */
void* pmm_alloc_frame(void) {
    uint32_t frame = first_free_frame();
    if (frame == 0 && test_frame(0)) {
        return NULL; /* Out of memory */
    }
    set_frame(frame);
    used_frames++;
    return (void*)frame;
}

/**
 * Free a physical frame
 */
void pmm_free_frame(void *addr) {
    uint32_t frame = (uint32_t)addr;
    if (!test_frame(frame)) return;
    clear_frame(frame);
    used_frames--;
}

/**
 * Get total memory
 */
uint32_t pmm_get_total_memory(void) {
    return mem_size_kb;
}

/**
 * Get used memory
 */
uint32_t pmm_get_used_memory(void) {
    return (used_frames * PAGE_SIZE) / 1024;
}
