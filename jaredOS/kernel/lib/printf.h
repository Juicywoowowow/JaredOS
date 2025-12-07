/**
 * jaredOS - Printf Header
 */

#ifndef PRINTF_H
#define PRINTF_H

#include "../types.h"

/* Kernel printf */
int kprintf(const char *format, ...);

/* Sprintf to buffer */
int ksprintf(char *buffer, const char *format, ...);

#endif /* PRINTF_H */
