/**
 * jaredOS - Timer Driver Implementation (PIT)
 */

#include "timer.h"
#include "../core/irq.h"
#include "../types.h"

/* PIT ports */
#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43

/* PIT frequency */
#define PIT_FREQUENCY   1193180

/* Timer state */
static volatile uint32_t tick_count = 0;
static uint32_t timer_frequency = 0;

/**
 * Timer interrupt handler
 */
static void timer_handler(registers_t *regs) {
    (void)regs;
    tick_count++;
}

/**
 * Initialize PIT timer
 */
void timer_init(uint32_t frequency) {
    timer_frequency = frequency;

    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;

    /* Set command byte: channel 0, lobyte/hibyte, rate generator */
    outb(PIT_COMMAND, 0x36);

    /* Send divisor */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    /* Register handler */
    irq_register_handler(0, timer_handler);
}

/**
 * Get tick count
 */
uint32_t timer_get_ticks(void) {
    return tick_count;
}

/**
 * Wait for specified milliseconds
 */
void timer_wait(uint32_t ms) {
    uint32_t ticks_to_wait = (ms * timer_frequency) / 1000;
    uint32_t start = tick_count;
    while ((tick_count - start) < ticks_to_wait) {
        __asm__ volatile ("hlt");
    }
}

/**
 * Get uptime in seconds
 */
uint32_t timer_get_uptime(void) {
    if (timer_frequency == 0) return 0;
    return tick_count / timer_frequency;
}
