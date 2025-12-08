/*
 * =============================================================================
 * types.h - Common Type Definitions and Utilities for FoxOS
 * =============================================================================
 *
 * This header provides fundamental type definitions, macros, and inline
 * functions used throughout the FoxOS kernel. It's the foundation that
 * all other source files build upon.
 *
 * DEBUGGING TIP: If you're seeing weird behavior, check that you're using
 * the correct sized types (uint8_t vs uint32_t). Off-by-one in size can
 * cause memory corruption that's hard to track down.
 *
 * =============================================================================
 */

#ifndef TYPES_H
#define TYPES_H

/* =============================================================================
 * SECTION 1: Fixed-Width Integer Types
 * =============================================================================
 * We define our own types since we can't use <stdint.h> in a freestanding env.
 * These match the sizes expected on i686 (32-bit x86).
 */

typedef unsigned char uint8_t;       /* 8-bit unsigned  (0 to 255) */
typedef unsigned short uint16_t;     /* 16-bit unsigned (0 to 65535) */
typedef unsigned int uint32_t;       /* 32-bit unsigned (0 to 4294967295) */
typedef unsigned long long uint64_t; /* 64-bit unsigned */

typedef signed char int8_t;       /* 8-bit signed  (-128 to 127) */
typedef signed short int16_t;     /* 16-bit signed (-32768 to 32767) */
typedef signed int int32_t;       /* 32-bit signed */
typedef signed long long int64_t; /* 64-bit signed */

typedef uint32_t size_t; /* Size type for memory operations */
typedef int32_t ssize_t; /* Signed size type */

/* =============================================================================
 * SECTION 2: Boolean Type
 * =============================================================================
 * In C23, bool/true/false are built-in keywords, nothing to define here.
 * For older compilers, uncomment the typedefs below.
 */

/* Uncomment for pre-C23 compilers:
typedef unsigned char bool;
#define true 1
#define false 0
*/

/* =============================================================================
 * SECTION 3: NULL Pointer
 * =============================================================================
 */

#define NULL ((void *)0)

/* =============================================================================
 * SECTION 4: Useful Macros
 * =============================================================================
 */

/* Get the minimum/maximum of two values */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Absolute value */
#define ABS(x) ((x) < 0 ? -(x) : (x))

/* Array element count */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Align value up to boundary (boundary must be power of 2) */
#define ALIGN_UP(val, align) (((val) + (align) - 1) & ~((align) - 1))

/* Check if a bit is set */
#define BIT_SET(val, bit) ((val) & (1 << (bit)))
#define SET_BIT(val, bit) ((val) | (1 << (bit)))
#define CLEAR_BIT(val, bit) ((val) & ~(1 << (bit)))

/* =============================================================================
 * SECTION 5: Port I/O Functions
 * =============================================================================
 * These inline assembly functions let us talk to hardware ports.
 * Essential for keyboard, mouse, VGA, and other hardware interaction.
 *
 * DEBUGGING TIP: If hardware isn't responding, double-check the port number.
 * Common ports:
 *   0x60 - PS/2 data port (keyboard/mouse)
 *   0x64 - PS/2 command/status port
 *   0x3D4/0x3D5 - VGA CRTC registers
 *   0x20/0x21 - Master PIC
 *   0xA0/0xA1 - Slave PIC
 */

/* Read a byte from an I/O port */
static inline uint8_t inb(uint16_t port) {
  uint8_t result;
  __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
  return result;
}

/* Write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Read a word (16-bit) from an I/O port */
static inline uint16_t inw(uint16_t port) {
  uint16_t result;
  __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
  return result;
}

/* Write a word (16-bit) to an I/O port */
static inline void outw(uint16_t port, uint16_t value) {
  __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/* Small delay - useful after port I/O for slow hardware */
static inline void io_wait(void) {
  /* Write to an unused port to create a small delay */
  outb(0x80, 0);
}

/* =============================================================================
 * SECTION 6: Interrupt Control
 * =============================================================================
 */

/* Disable interrupts */
static inline void cli(void) { __asm__ volatile("cli"); }

/* Enable interrupts */
static inline void sti(void) { __asm__ volatile("sti"); }

/* Halt the CPU until next interrupt */
static inline void hlt(void) { __asm__ volatile("hlt"); }

/* =============================================================================
 * SECTION 7: Memory Operations
 * =============================================================================
 * Basic memory functions since we can't use libc.
 */

/* Set memory to a value */
static inline void *memset(void *dest, int val, size_t count) {
  uint8_t *d = (uint8_t *)dest;
  while (count--) {
    *d++ = (uint8_t)val;
  }
  return dest;
}

/* Copy memory */
static inline void *memcpy(void *dest, const void *src, size_t count) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;
  while (count--) {
    *d++ = *s++;
  }
  return dest;
}

/* Compare memory */
static inline int memcmp(const void *s1, const void *s2, size_t count) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;
  while (count--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

/* Get string length */
static inline size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len]) {
    len++;
  }
  return len;
}

/* =============================================================================
 * SECTION 8: Debug Macros
 * =============================================================================
 * QEMU can output to a serial port or debug console.
 * Write to port 0xE9 to output to QEMU's debug console (if enabled).
 *
 * DEBUGGING TIP: Run QEMU with -debugcon stdio to see debug output.
 * Example: qemu-system-i386 -fda foxos.img -debugcon stdio
 */

#define DEBUG_PORT 0xE9

/* Output a character to QEMU debug console */
static inline void debug_putchar(char c) { outb(DEBUG_PORT, c); }

/* Output a string to QEMU debug console */
static inline void debug_print(const char *str) {
  while (*str) {
    debug_putchar(*str++);
  }
}

/* Output a hex value to QEMU debug console */
static inline void debug_hex(uint32_t val) {
  const char *hex = "0123456789ABCDEF";
  debug_print("0x");
  for (int i = 28; i >= 0; i -= 4) {
    debug_putchar(hex[(val >> i) & 0xF]);
  }
}

#endif /* TYPES_H */
