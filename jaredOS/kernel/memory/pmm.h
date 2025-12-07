/**
 * jaredOS - Physical Memory Manager Header
 */

#ifndef PMM_H
#define PMM_H

#include "../types.h"

/* Page size (4KB) */
#define PAGE_SIZE 4096

/* Initialize PMM */
void pmm_init(uint32_t mem_size);

/* Allocate a physical frame */
void* pmm_alloc_frame(void);

/* Free a physical frame */
void pmm_free_frame(void *addr);

/* Get total/used memory */
uint32_t pmm_get_total_memory(void);
uint32_t pmm_get_used_memory(void);

#endif /* PMM_H */
