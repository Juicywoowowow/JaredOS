/*
 * =============================================================================
 * SharkOS Port I/O Header (io.h)
 * =============================================================================
 * Low-level port I/O functions for communicating with hardware.
 * x86 uses I/O ports to talk to devices like keyboard, VGA, disk, etc.
 *
 * DEBUGGING TIPS:
 *   - If hardware isn't responding, check if you're using the right port number
 *   - Some devices need delays between I/O operations
 *   - Common ports: 0x60/0x64 = keyboard, 0x3D4/0x3D5 = VGA cursor
 * =============================================================================
 */

#ifndef IO_H
#define IO_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * outb - Write a byte to an I/O port
 * 
 * @param port: The I/O port number (0-65535)
 * @param value: The byte to write (0-255)
 *
 * Example: outb(0x3D4, 0x0F);  // Write to VGA cursor register
 * ---------------------------------------------------------------------------- */
static inline void outb(uint16_t port, uint8_t value) {
    /* 
     * Inline assembly explanation:
     *   "out %1, %0" - x86 OUT instruction
     *   %0 = 'a' constraint = AL register = value
     *   %1 = 'Nd' constraint = DX register or immediate = port
     */
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* ----------------------------------------------------------------------------
 * inb - Read a byte from an I/O port
 *
 * @param port: The I/O port number
 * @return: The byte read from the port
 *
 * Example: uint8_t key = inb(0x60);  // Read from keyboard data port
 * ---------------------------------------------------------------------------- */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* ----------------------------------------------------------------------------
 * outw - Write a word (16-bit) to an I/O port
 * ---------------------------------------------------------------------------- */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/* ----------------------------------------------------------------------------
 * inw - Read a word (16-bit) from an I/O port
 * ---------------------------------------------------------------------------- */
static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* ----------------------------------------------------------------------------
 * io_wait - Wait for I/O operation to complete
 * 
 * Some older hardware needs a short delay between I/O operations.
 * We do this by writing to port 0x80 (unused POST diagnostic port).
 * ---------------------------------------------------------------------------- */
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif /* IO_H */
