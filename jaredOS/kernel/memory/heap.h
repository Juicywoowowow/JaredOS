/**
 * jaredOS - Kernel Heap Header
 */

#ifndef HEAP_H
#define HEAP_H

#include "../types.h"

/* Initialize heap */
void heap_init(void);

/* Allocate memory */
void* kmalloc(size_t size);

/* Allocate aligned memory */
void* kmalloc_aligned(size_t size, size_t alignment);

/* Free memory */
void kfree(void *ptr);

/* Get free heap size */
size_t heap_free_size(void);

#endif /* HEAP_H */
