/*
 * =============================================================================
 * SharkOS Memory Utilities Header (memory.h)
 * =============================================================================
 * Basic memory manipulation functions.
 * We implement these ourselves since we don't have a standard library.
 * =============================================================================
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * memset - Fill memory with a constant byte
 *
 * @param dest: Pointer to memory to fill
 * @param val: Byte value to fill with
 * @param count: Number of bytes to fill
 * @return: Pointer to dest
 * ---------------------------------------------------------------------------- */
void* memset(void* dest, int val, size_t count);

/* ----------------------------------------------------------------------------
 * memcpy - Copy memory from source to destination
 *
 * @param dest: Destination pointer
 * @param src: Source pointer
 * @param count: Number of bytes to copy
 * @return: Pointer to dest
 *
 * WARNING: Does not handle overlapping memory! Use memmove for that.
 * ---------------------------------------------------------------------------- */
void* memcpy(void* dest, const void* src, size_t count);

/* ----------------------------------------------------------------------------
 * memcmp - Compare two memory regions
 *
 * @param s1: First memory region
 * @param s2: Second memory region
 * @param count: Number of bytes to compare
 * @return: 0 if equal, <0 if s1<s2, >0 if s1>s2
 * ---------------------------------------------------------------------------- */
int memcmp(const void* s1, const void* s2, size_t count);

#endif /* MEMORY_H */
