/**
 * jaredOS - Timer Driver Header
 */

#ifndef TIMER_H
#define TIMER_H

#include "../types.h"

/* Initialize timer with given frequency (Hz) */
void timer_init(uint32_t frequency);

/* Get tick count since boot */
uint32_t timer_get_ticks(void);

/* Wait for specified milliseconds */
void timer_wait(uint32_t ms);

/* Get uptime in seconds */
uint32_t timer_get_uptime(void);

#endif /* TIMER_H */
