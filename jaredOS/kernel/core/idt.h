/**
 * jaredOS - Interrupt Descriptor Table Header
 */

#ifndef IDT_H
#define IDT_H

#include "../types.h"

/* IDT entry structure */
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

/* IDT pointer structure */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* Initialize IDT */
void idt_init(void);

/* Set an IDT entry */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

/* External assembly function */
extern void idt_load(uint32_t);

#endif /* IDT_H */
