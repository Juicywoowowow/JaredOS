/**
 * jaredOS - Serial Port Driver Header
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "../types.h"

/* Serial ports */
#define COM1 0x3F8
#define COM2 0x2F8

/* Initialize serial port */
void serial_init(void);

/* Write character to serial */
void serial_putchar(char c);

/* Write string to serial */
void serial_puts(const char *str);

/* Read character from serial */
char serial_getchar(void);

/* Check if data is available */
bool serial_has_data(void);

#endif /* SERIAL_H */
