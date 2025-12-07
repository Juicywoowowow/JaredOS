/**
 * jaredOS - Kernel Heap Implementation
 */

#include "heap.h"
#include "../lib/string.h"

/* Heap configuration */
#define HEAP_START  0x400000    /* 4MB */
#define HEAP_SIZE   0x100000    /* 1MB heap */
#define HEAP_END    (HEAP_START + HEAP_SIZE)

/* Block header structure */
typedef struct block_header {
    size_t size;
    uint8_t is_free;
    struct block_header *next;
} block_header_t;

/* First block */
static block_header_t *heap_start = NULL;

/**
 * Initialize heap
 */
void heap_init(void) {
    heap_start = (block_header_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(block_header_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
}

/**
 * Find a free block of sufficient size
 */
static block_header_t* find_free_block(size_t size) {
    block_header_t *current = heap_start;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Split block if it's larger than needed
 */
static void split_block(block_header_t *block, size_t size) {
    if (block->size > size + sizeof(block_header_t) + 16) {
        block_header_t *new_block = (block_header_t*)((uint8_t*)block + sizeof(block_header_t) + size);
        new_block->size = block->size - size - sizeof(block_header_t);
        new_block->is_free = 1;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;
    }
}

/**
 * Allocate memory
 */
void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Align to 4 bytes */
    if (size % 4 != 0) {
        size += 4 - (size % 4);
    }

    block_header_t *block = find_free_block(size);
    if (block == NULL) {
        return NULL; /* Out of memory */
    }

    split_block(block, size);
    block->is_free = 0;

    return (void*)((uint8_t*)block + sizeof(block_header_t));
}

/**
 * Allocate aligned memory
 */
void* kmalloc_aligned(size_t size, size_t alignment) {
    /* Simple implementation: allocate extra and align */
    void *ptr = kmalloc(size + alignment);
    if (ptr == NULL) return NULL;

    uint32_t addr = (uint32_t)ptr;
    if (addr % alignment != 0) {
        addr += alignment - (addr % alignment);
    }
    return (void*)addr;
}

/**
 * Merge adjacent free blocks
 */
static void merge_free_blocks(void) {
    block_header_t *current = heap_start;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(block_header_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

/**
 * Free memory
 */
void kfree(void *ptr) {
    if (ptr == NULL) return;

    block_header_t *block = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));

    /* Validate pointer is within heap */
    if ((uint32_t)block < HEAP_START || (uint32_t)block >= HEAP_END) {
        return;
    }

    block->is_free = 1;
    merge_free_blocks();
}

/**
 * Get free heap size
 */
size_t heap_free_size(void) {
    size_t free_size = 0;
    block_header_t *current = heap_start;
    while (current != NULL) {
        if (current->is_free) {
            free_size += current->size;
        }
        current = current->next;
    }
    return free_size;
}
