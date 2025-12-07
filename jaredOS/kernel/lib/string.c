/**
 * jaredOS - String Functions Implementation
 */

#include "string.h"

/**
 * Get string length
 */
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

/**
 * Compare two strings
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/**
 * Compare n characters of two strings
 */
int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/**
 * Copy string
 */
char* strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

/**
 * Copy n characters
 */
char* strncpy(char *dest, const char *src, size_t n) {
    char *ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

/**
 * Concatenate strings
 */
char* strcat(char *dest, const char *src) {
    char *ret = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return ret;
}

/**
 * Find character in string
 */
char* strchr(const char *str, int c) {
    while (*str) {
        if (*str == (char)c) return (char*)str;
        str++;
    }
    return (c == '\0') ? (char*)str : NULL;
}

/**
 * Find last occurrence of character
 */
char* strrchr(const char *str, int c) {
    const char *last = NULL;
    while (*str) {
        if (*str == (char)c) last = str;
        str++;
    }
    return (c == '\0') ? (char*)str : (char*)last;
}

/**
 * Set memory
 */
void* memset(void *ptr, int value, size_t num) {
    uint8_t *p = (uint8_t*)ptr;
    while (num--) *p++ = (uint8_t)value;
    return ptr;
}

/**
 * Copy memory
 */
void* memcpy(void *dest, const void *src, size_t num) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    while (num--) *d++ = *s++;
    return dest;
}

/**
 * Compare memory
 */
int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const uint8_t *p1 = (const uint8_t*)ptr1;
    const uint8_t *p2 = (const uint8_t*)ptr2;
    while (num--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

/**
 * Move memory (handles overlap)
 */
void* memmove(void *dest, const void *src, size_t num) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    
    if (d < s) {
        while (num--) *d++ = *s++;
    } else {
        d += num;
        s += num;
        while (num--) *--d = *--s;
    }
    return dest;
}
