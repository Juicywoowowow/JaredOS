/**
 * jaredOS - Hardware IRQ Header
 */

#ifndef IRQ_H
#define IRQ_H

#include "../types.h"
#include "isr.h"

/* Initialize IRQs */
void irq_init(void);

/* Register an IRQ handler */
void irq_register_handler(uint8_t irq, isr_handler_t handler);

/* IRQ stubs (defined in irq_asm.asm) */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

#endif /* IRQ_H */
