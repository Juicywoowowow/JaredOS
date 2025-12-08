/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Memory Management (Arena Allocator)
 * 
 * Provides efficient memory allocation with per-context cleanup.
 * Uses arena allocation for fast allocation and bulk deallocation.
 */

#ifndef JSBOX_MEMORY_H
#define JSBOX_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Arena Allocator
 * ============================================================================ */

#define JSBOX_ARENA_DEFAULT_SIZE (64 * 1024)  /* 64KB default block */
#define JSBOX_ARENA_ALIGNMENT    8

typedef struct JSBox_ArenaBlock {
    struct JSBox_ArenaBlock* next;
    size_t size;
    size_t used;
    uint8_t data[];
} JSBox_ArenaBlock;

typedef struct JSBox_Arena {
    JSBox_ArenaBlock* current;
    JSBox_ArenaBlock* first;
    size_t total_allocated;
    size_t block_count;
} JSBox_Arena;

/* Create a new arena with default initial size */
JSBox_Arena* jsbox_arena_create(void);

/* Create arena with custom initial size */
JSBox_Arena* jsbox_arena_create_sized(size_t initial_size);

/* Allocate memory from arena (never returns NULL, aborts on OOM) */
void* jsbox_arena_alloc(JSBox_Arena* arena, size_t size);

/* Allocate zeroed memory */
void* jsbox_arena_calloc(JSBox_Arena* arena, size_t count, size_t size);

/* Duplicate a string into the arena */
char* jsbox_arena_strdup(JSBox_Arena* arena, const char* str);

/* Duplicate n bytes of string into arena (null-terminated) */
char* jsbox_arena_strndup(JSBox_Arena* arena, const char* str, size_t n);

/* Reset arena (keeps memory, resets usage) */
void jsbox_arena_reset(JSBox_Arena* arena);

/* Destroy arena and free all memory */
void jsbox_arena_destroy(JSBox_Arena* arena);

/* Get total bytes allocated */
size_t jsbox_arena_total_allocated(const JSBox_Arena* arena);

/* Get total bytes used */
size_t jsbox_arena_total_used(const JSBox_Arena* arena);

/* ============================================================================
 * General Memory Utilities
 * ============================================================================ */

/* Safe malloc that aborts on failure */
void* jsbox_malloc(size_t size);

/* Safe calloc that aborts on failure */
void* jsbox_calloc(size_t count, size_t size);

/* Safe realloc that aborts on failure */
void* jsbox_realloc(void* ptr, size_t size);

/* Free memory (wrapper for consistency) */
void jsbox_free(void* ptr);

/* Duplicate string (heap allocated) */
char* jsbox_strdup(const char* str);

#endif /* JSBOX_MEMORY_H */
