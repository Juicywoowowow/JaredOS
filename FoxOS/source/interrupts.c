/*
 * =============================================================================
 * interrupts.c - IRQ Handlers and Timer for FoxOS
 * =============================================================================
 *
 * This file handles hardware interrupts and the system timer.
 * The Programmable Interval Timer (PIT) generates periodic interrupts
 * that we use for timing, animations, and task switching.
 *
 * DEBUGGING TIPS:
 *   - If timer_ticks isn't incrementing, PIC remapping may have failed
 *   - IRQ0 (timer) fires at whatever frequency you set
 *   - Use QEMU's -d int flag to verify interrupts are firing
 *   - Check that sti() is called after initialization
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: Timer (PIT - Programmable Interval Timer)
 * =============================================================================
 *
 * The PIT runs at 1.193182 MHz. We program it to generate interrupts
 * at a specified frequency (typically 100 Hz for OS work).
 */

#define PIT_CHANNEL0 0x40     /* Channel 0 data port */
#define PIT_COMMAND 0x43      /* Mode/Command register */
#define PIT_FREQUENCY 1193182 /* Base frequency in Hz */

/* Global tick counter - increments on every timer interrupt */
volatile uint32_t timer_ticks = 0;

/* Timer frequency in Hz */
static uint32_t timer_frequency = 100;

/*
 * timer_init - Initialize the Programmable Interval Timer
 *
 * @param freq: Desired interrupt frequency in Hz (e.g., 100)
 *
 * DEBUGGING TIP: If freq is too high (>1000), the system might become
 * sluggish due to interrupt overhead.
 */
void timer_init(uint32_t freq) {
  timer_frequency = freq;

  /* Calculate the divisor */
  uint16_t divisor = (uint16_t)(PIT_FREQUENCY / freq);

  /* Send command byte:
   * Bits 7-6: Channel 0
   * Bits 5-4: Access mode (lobyte/hibyte)
   * Bits 3-1: Mode 3 (square wave generator)
   * Bit 0: Binary mode
   */
  outb(PIT_COMMAND, 0x36);

  /* Send divisor (low byte first, then high byte) */
  outb(PIT_CHANNEL0, divisor & 0xFF);
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

  debug_print("[TIMER] PIT initialized at ");
  debug_hex(freq);
  debug_print(" Hz (divisor: ");
  debug_hex(divisor);
  debug_print(")\n");
}

/*
 * timer_handler - Called on every timer interrupt (IRQ0)
 *
 * This is called from the IRQ handler in kernelA.c.
 * Keep this function fast - it runs in interrupt context!
 */
void timer_handler(void) {
  timer_ticks++;

/* Debug output every second (at 100 Hz, that's every 100 ticks) */
#if 0 /* Enable for debugging */
    if (timer_ticks % timer_frequency == 0) {
        debug_print("[TIMER] Tick: ");
        debug_hex(timer_ticks);
        debug_print("\n");
    }
#endif
}

/*
 * timer_get_ticks - Get current tick count
 *
 * Returns: Number of timer ticks since boot
 */
uint32_t timer_get_ticks(void) { return timer_ticks; }

/*
 * timer_get_seconds - Get approximate seconds since boot
 *
 * Returns: Seconds elapsed (based on timer frequency)
 */
uint32_t timer_get_seconds(void) { return timer_ticks / timer_frequency; }

/*
 * timer_sleep - Busy-wait for specified number of ticks
 *
 * @param ticks: Number of ticks to wait
 *
 * WARNING: This is a busy-wait that blocks the CPU!
 * Only use for short delays. For longer waits, implement proper scheduling.
 */
void timer_sleep(uint32_t ticks) {
  uint32_t end_tick = timer_ticks + ticks;
  while (timer_ticks < end_tick) {
    /* Busy wait - CPU spins here */
    hlt(); /* At least save some power while waiting */
  }
}

/*
 * timer_sleep_ms - Sleep for specified milliseconds
 *
 * @param ms: Milliseconds to sleep
 */
void timer_sleep_ms(uint32_t ms) {
  uint32_t ticks = (ms * timer_frequency) / 1000;
  if (ticks == 0)
    ticks = 1; /* At least 1 tick */
  timer_sleep(ticks);
}

/* =============================================================================
 * SECTION 2: System Uptime and Clock
 * =============================================================================
 */

/*
 * Time structure for simple clock display
 */
typedef struct {
  uint32_t hours;
  uint32_t minutes;
  uint32_t seconds;
} uptime_t;

/*
 * timer_get_uptime - Get system uptime in human-readable format
 *
 * @param uptime: Pointer to uptime_t structure to fill
 */
void timer_get_uptime(uptime_t *uptime) {
  uint32_t total_seconds = timer_get_seconds();

  uptime->hours = total_seconds / 3600;
  uptime->minutes = (total_seconds % 3600) / 60;
  uptime->seconds = total_seconds % 60;
}

/* =============================================================================
 * SECTION 3: Delay Utilities
 * =============================================================================
 */

/*
 * delay - Simple delay loop (not precise, but doesn't need timer)
 *
 * @param count: Number of loop iterations
 *
 * DEBUGGING TIP: The actual delay depends on CPU speed.
 * Use timer_sleep_ms for more precise delays.
 */
void delay(uint32_t count) {
  while (count--) {
    __asm__ volatile("nop");
  }
}

/*
 * io_delay - Very short delay for hardware I/O timing
 *
 * Some hardware needs a brief delay between port operations.
 * This writes to an unused port to create that delay.
 */
void io_delay(void) {
  outb(0x80, 0); /* Port 0x80 is unused on modern systems */
}
