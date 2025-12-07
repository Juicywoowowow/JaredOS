/**
 * jaredOS - Interrupt Descriptor Table Implementation
 */

#include "idt.h"
#include "../lib/string.h"

/* IDT with 256 entries */
static struct idt_entry idt[256];
static struct idt_ptr idt_pointer;

/**
 * Set an IDT entry
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = sel;
    idt[num].zero      = 0;
    idt[num].flags     = flags;
}

/**
 * Initialize the IDT
 */
void idt_init(void) {
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base  = (uint32_t)&idt;

    /* Clear IDT */
    memset(&idt, 0, sizeof(idt));

    /* Load IDT */
    idt_load((uint32_t)&idt_pointer);
}
