/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Memory Management Implementation
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static void jsbox_oom_abort(size_t size) {
    fprintf(stderr, "JSBOX: Out of memory (requested %zu bytes)\n", size);
    abort();
}

static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static JSBox_ArenaBlock* arena_block_create(size_t size) {
    size_t total = sizeof(JSBox_ArenaBlock) + size;
    JSBox_ArenaBlock* block = (JSBox_ArenaBlock*)malloc(total);
    if (!block) {
        jsbox_oom_abort(total);
    }
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

/* ============================================================================
 * Arena Allocator Implementation
 * ============================================================================ */

JSBox_Arena* jsbox_arena_create(void) {
    return jsbox_arena_create_sized(JSBOX_ARENA_DEFAULT_SIZE);
}

JSBox_Arena* jsbox_arena_create_sized(size_t initial_size) {
    JSBox_Arena* arena = (JSBox_Arena*)malloc(sizeof(JSBox_Arena));
    if (!arena) {
        jsbox_oom_abort(sizeof(JSBox_Arena));
    }
    
    JSBox_ArenaBlock* block = arena_block_create(initial_size);
    
    arena->current = block;
    arena->first = block;
    arena->total_allocated = initial_size;
    arena->block_count = 1;
    
    return arena;
}

void* jsbox_arena_alloc(JSBox_Arena* arena, size_t size) {
    if (size == 0) {
        size = 1;
    }
    
    size = align_up(size, JSBOX_ARENA_ALIGNMENT);
    
    /* Check if current block has space */
    JSBox_ArenaBlock* block = arena->current;
    if (block->used + size > block->size) {
        /* Need a new block */
        size_t new_size = block->size * 2;
        if (new_size < size) {
            new_size = size;
        }
        
        JSBox_ArenaBlock* new_block = arena_block_create(new_size);
        block->next = new_block;
        arena->current = new_block;
        arena->total_allocated += new_size;
        arena->block_count++;
        block = new_block;
    }
    
    void* ptr = block->data + block->used;
    block->used += size;
    return ptr;
}

void* jsbox_arena_calloc(JSBox_Arena* arena, size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = jsbox_arena_alloc(arena, total);
    memset(ptr, 0, total);
    return ptr;
}

char* jsbox_arena_strdup(JSBox_Arena* arena, const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* dup = (char*)jsbox_arena_alloc(arena, len + 1);
    memcpy(dup, str, len + 1);
    return dup;
}

char* jsbox_arena_strndup(JSBox_Arena* arena, const char* str, size_t n) {
    if (!str) return NULL;
    size_t len = strlen(str);
    if (len > n) len = n;
    char* dup = (char*)jsbox_arena_alloc(arena, len + 1);
    memcpy(dup, str, len);
    dup[len] = '\0';
    return dup;
}

void jsbox_arena_reset(JSBox_Arena* arena) {
    /* Free all blocks except the first */
    JSBox_ArenaBlock* block = arena->first->next;
    while (block) {
        JSBox_ArenaBlock* next = block->next;
        free(block);
        block = next;
    }
    
    arena->first->next = NULL;
    arena->first->used = 0;
    arena->current = arena->first;
    arena->total_allocated = arena->first->size;
    arena->block_count = 1;
}

void jsbox_arena_destroy(JSBox_Arena* arena) {
    if (!arena) return;
    
    JSBox_ArenaBlock* block = arena->first;
    while (block) {
        JSBox_ArenaBlock* next = block->next;
        free(block);
        block = next;
    }
    
    free(arena);
}

size_t jsbox_arena_total_allocated(const JSBox_Arena* arena) {
    return arena->total_allocated;
}

size_t jsbox_arena_total_used(const JSBox_Arena* arena) {
    size_t used = 0;
    JSBox_ArenaBlock* block = arena->first;
    while (block) {
        used += block->used;
        block = block->next;
    }
    return used;
}

/* ============================================================================
 * General Memory Utilities
 * ============================================================================ */

void* jsbox_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && size > 0) {
        jsbox_oom_abort(size);
    }
    return ptr;
}

void* jsbox_calloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (!ptr && count > 0 && size > 0) {
        jsbox_oom_abort(count * size);
    }
    return ptr;
}

void* jsbox_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        jsbox_oom_abort(size);
    }
    return new_ptr;
}

void jsbox_free(void* ptr) {
    free(ptr);
}

char* jsbox_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* dup = (char*)jsbox_malloc(len + 1);
    memcpy(dup, str, len + 1);
    return dup;
}
