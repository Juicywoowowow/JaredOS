/**
 * jaredOS - Type Definitions
 */

#ifndef TYPES_H
#define TYPES_H

/* Unsigned integers */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

/* Signed integers */
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

/* Size types */
typedef uint32_t size_t;
typedef int32_t  ssize_t;
typedef int32_t  ptrdiff_t;

/* Boolean - use _Bool to avoid C23 keyword issues */
typedef _Bool bool;
#define true 1
#define false 0

/* NULL pointer */
#define NULL ((void*)0)

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif /* TYPES_H */
