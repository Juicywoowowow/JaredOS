/**
 * jaredOS - Global Descriptor Table Implementation
 */

#include "gdt.h"

/* GDT with 3 entries: null, code, data */
static struct gdt_entry gdt[3];
static struct gdt_ptr gdt_pointer;

/**
 * Set a GDT entry
 */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

/**
 * Initialize the GDT
 */
void gdt_init(void) {
    gdt_pointer.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gdt_pointer.base  = (uint32_t)&gdt;

    /* Null segment */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Kernel code segment: base=0, limit=4GB, code, ring 0 */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Kernel data segment: base=0, limit=4GB, data, ring 0 */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* Load the GDT */
    gdt_flush((uint32_t)&gdt_pointer);
}
