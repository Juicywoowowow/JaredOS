/**
 * jaredOS - Virtual Memory Manager Header
 */

#ifndef VMM_H
#define VMM_H

#include "../types.h"

/* Page flags */
#define PAGE_PRESENT    0x01
#define PAGE_WRITE      0x02
#define PAGE_USER       0x04

/* Initialize VMM */
void vmm_init(void);

/* Map a virtual address to physical */
void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/* Unmap a virtual address */
void vmm_unmap_page(uint32_t virt);

/* Get physical address for virtual address */
uint32_t vmm_get_physical(uint32_t virt);

#endif /* VMM_H */
