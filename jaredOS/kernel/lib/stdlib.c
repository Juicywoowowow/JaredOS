/**
 * jaredOS - Standard Library Functions Implementation
 */

#include "stdlib.h"

/**
 * Integer to string
 */
char* itoa(int value, char *str, int base) {
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    int tmp_value;

    /* Handle negative numbers for base 10 */
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        ptr1++;
        value = -value;
    }

    /* Process individual digits */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

/**
 * Unsigned integer to string
 */
char* utoa(uint32_t value, char *str, int base) {
    char *ptr = str;
    char *ptr1 = str;
    char tmp_char;
    uint32_t tmp_value;

    /* Process individual digits */
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    /* Reverse the string */
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

/**
 * String to integer
 */
int atoi(const char *str) {
    int result = 0;
    int sign = 1;

    /* Skip whitespace */
    while (isspace(*str)) str++;

    /* Handle sign */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    /* Process digits */
    while (isdigit(*str)) {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

/**
 * Absolute value
 */
int abs(int n) {
    return (n < 0) ? -n : n;
}

/**
 * Check if character is digit
 */
int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

/**
 * Check if character is alpha
 */
int isalpha(int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/**
 * Check if character is alphanumeric
 */
int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

/**
 * Check if character is whitespace
 */
int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

/**
 * Convert to uppercase
 */
int toupper(int c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

/**
 * Convert to lowercase
 */
int tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}
