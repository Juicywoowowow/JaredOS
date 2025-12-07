/**
 * jaredOS - Standard Library Functions Header
 */

#ifndef STDLIB_H
#define STDLIB_H

#include "../types.h"

/* Integer to string */
char* itoa(int value, char *str, int base);

/* Unsigned integer to string */
char* utoa(uint32_t value, char *str, int base);

/* String to integer */
int atoi(const char *str);

/* Absolute value */
int abs(int n);

/* Check if character is digit */
int isdigit(int c);

/* Check if character is alpha */
int isalpha(int c);

/* Check if character is alphanumeric */
int isalnum(int c);

/* Check if character is whitespace */
int isspace(int c);

/* Convert to uppercase */
int toupper(int c);

/* Convert to lowercase */
int tolower(int c);

#endif /* STDLIB_H */
