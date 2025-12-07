/**
 * jaredOS - Virtual Memory Manager Implementation
 */

#include "vmm.h"
#include "pmm.h"
#include "../lib/string.h"

/* Page directory and tables */
static uint32_t *page_directory = NULL;
static uint32_t *page_tables = NULL;

/* Addresses */
#define PAGE_DIR_ADDR   0x300000    /* 3MB */
#define PAGE_TABLE_ADDR 0x301000    /* After page directory */

/**
 * Get page table entry for a virtual address
 */
static uint32_t* get_page_entry(uint32_t virt) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        return NULL;
    }

    uint32_t *page_table = (uint32_t*)(page_directory[pd_idx] & 0xFFFFF000);
    return &page_table[pt_idx];
}

/**
 * Initialize VMM with identity mapping
 */
void vmm_init(void) {
    page_directory = (uint32_t*)PAGE_DIR_ADDR;
    page_tables = (uint32_t*)PAGE_TABLE_ADDR;

    /* Clear page directory */
    memset(page_directory, 0, PAGE_SIZE);

    /* Identity map first 4MB (kernel space) */
    uint32_t *first_table = page_tables;
    for (uint32_t i = 0; i < 1024; i++) {
        first_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }

    /* Set page directory entry */
    page_directory[0] = (uint32_t)first_table | PAGE_PRESENT | PAGE_WRITE;

    /* Enable paging */
    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_directory) : "eax"
    );
}

/**
 * Map a virtual address to physical
 */
void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    /* Create page table if not present */
    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        uint32_t *new_table = (uint32_t*)pmm_alloc_frame();
        if (!new_table) return;
        memset(new_table, 0, PAGE_SIZE);
        page_directory[pd_idx] = (uint32_t)new_table | PAGE_PRESENT | PAGE_WRITE;
    }

    uint32_t *page_table = (uint32_t*)(page_directory[pd_idx] & 0xFFFFF000);
    page_table[pt_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;

    /* Invalidate TLB entry */
    __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
}

/**
 * Unmap a virtual address
 */
void vmm_unmap_page(uint32_t virt) {
    uint32_t *entry = get_page_entry(virt);
    if (entry) {
        *entry = 0;
        __asm__ volatile ("invlpg (%0)" : : "r"(virt) : "memory");
    }
}

/**
 * Get physical address for virtual address
 */
uint32_t vmm_get_physical(uint32_t virt) {
    uint32_t *entry = get_page_entry(virt);
    if (entry && (*entry & PAGE_PRESENT)) {
        return (*entry & 0xFFFFF000) | (virt & 0xFFF);
    }
    return 0;
}
