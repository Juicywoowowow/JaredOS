/**
 * jaredOS - Hardware IRQ Implementation
 */

#include "irq.h"
#include "idt.h"
#include "../types.h"

/* PIC ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* IRQ handlers */
static isr_handler_t irq_handlers[16] = {0};

/**
 * Remap the PIC to use interrupts 32-47
 */
static void pic_remap(void) {
    uint8_t mask1, mask2;

    /* Save masks */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    /* Initialize PICs */
    outb(PIC1_COMMAND, 0x11);   /* ICW1: init + ICW4 needed */
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    outb(PIC1_DATA, 0x20);      /* ICW2: IRQ 0-7 -> INT 32-39 */
    io_wait();
    outb(PIC2_DATA, 0x28);      /* ICW2: IRQ 8-15 -> INT 40-47 */
    io_wait();

    outb(PIC1_DATA, 0x04);      /* ICW3: slave PIC at IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02);      /* ICW3: cascade identity */
    io_wait();

    outb(PIC1_DATA, 0x01);      /* ICW4: 8086 mode */
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    /* Restore masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/**
 * Register an IRQ handler
 */
void irq_register_handler(uint8_t irq, isr_handler_t handler) {
    irq_handlers[irq] = handler;
}

/**
 * Common IRQ handler - called from assembly
 */
void irq_handler(registers_t *regs) {
    uint8_t irq = regs->int_no - 32;

    /* Call registered handler */
    if (irq_handlers[irq] != 0) {
        irq_handlers[irq](regs);
    }

    /* Send EOI (End of Interrupt) */
    if (regs->int_no >= 40) {
        outb(PIC2_COMMAND, 0x20);  /* EOI to slave PIC */
    }
    outb(PIC1_COMMAND, 0x20);      /* EOI to master PIC */
}

/**
 * Initialize IRQs
 */
void irq_init(void) {
    /* Remap PICs */
    pic_remap();

    /* Set up IRQ gates (32-47) */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}
