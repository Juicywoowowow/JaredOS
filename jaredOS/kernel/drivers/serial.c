/**
 * jaredOS - Serial Port Driver Implementation
 */

#include "serial.h"
#include "../types.h"

/* Serial port base */
static uint16_t serial_port = COM1;

/**
 * Check if transmit buffer is empty
 */
static int is_transmit_empty(void) {
    return inb(serial_port + 5) & 0x20;
}

/**
 * Check if data is available
 */
bool serial_has_data(void) {
    return inb(serial_port + 5) & 1;
}

/**
 * Initialize serial port (COM1)
 */
void serial_init(void) {
    outb(serial_port + 1, 0x00);    /* Disable interrupts */
    outb(serial_port + 3, 0x80);    /* Enable DLAB */
    outb(serial_port + 0, 0x03);    /* Set divisor to 3 (38400 baud) lo byte */
    outb(serial_port + 1, 0x00);    /*                            hi byte */
    outb(serial_port + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(serial_port + 2, 0xC7);    /* Enable FIFO, clear, 14-byte threshold */
    outb(serial_port + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}

/**
 * Write character to serial
 */
void serial_putchar(char c) {
    while (!is_transmit_empty());
    outb(serial_port, c);
}

/**
 * Write string to serial
 */
void serial_puts(const char *str) {
    while (*str) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str++);
    }
}

/**
 * Read character from serial
 */
char serial_getchar(void) {
    while (!serial_has_data());
    return inb(serial_port);
}
