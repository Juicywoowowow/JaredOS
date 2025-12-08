/*
 * =============================================================================
 * SharkOS String Utilities (string.c)
 * =============================================================================
 * Implementation of string manipulation functions.
 *
 * DEBUGGING TIPS:
 *   - Most crashes in string functions are due to NULL pointers or missing null terminators
 *   - Always check that your strings are properly null-terminated
 *   - Buffer overflows are silent killers - print string lengths when debugging
 * =============================================================================
 */

#include "string.h"
#include "memory.h"

/*
 * strlen - Get length of null-terminated string
 *
 * Counts characters until null terminator is reached.
 * Does NOT include the null terminator in the count.
 */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/*
 * strcmp - Compare two strings lexicographically
 *
 * Returns:
 *   0 if strings are equal
 *   <0 if s1 comes before s2
 *   >0 if s1 comes after s2
 */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/*
 * strncmp - Compare first n characters of two strings
 */
int strncmp(const char* s1, const char* s2, size_t n) {
    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/*
 * strcpy - Copy string from src to dest
 *
 * WARNING: Caller must ensure dest buffer is large enough!
 * Buffer overflow = potential security vulnerability + crashes
 */
char* strcpy(char* dest, const char* src) {
    char* original = dest;
    while ((*dest++ = *src++) != '\0');
    return original;
}

/*
 * strncpy - Safe(r) string copy with length limit
 *
 * NOTE: If src is shorter than n, dest is padded with null bytes.
 * If src is longer than n, dest is NOT null-terminated!
 */
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

/*
 * strcat - Concatenate src to end of dest
 *
 * Dest must have enough space for both strings plus null terminator.
 */
char* strcat(char* dest, const char* src) {
    char* ptr = dest + strlen(dest);
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

/*
 * strchr - Find first occurrence of character in string
 *
 * Also finds the null terminator if c == 0.
 */
char* strchr(const char* str, int c) {
    while (*str != '\0') {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    /* Check if we're looking for the null terminator */
    if ((char)c == '\0') {
        return (char*)str;
    }
    return NULL;
}

/*
 * strrchr - Find last occurrence of character in string
 *
 * Searches from the end of the string backwards.
 */
char* strrchr(const char* str, int c) {
    const char* last = NULL;
    
    while (*str != '\0') {
        if (*str == (char)c) {
            last = str;
        }
        str++;
    }
    /* Check if we're looking for the null terminator */
    if ((char)c == '\0') {
        return (char*)str;
    }
    return (char*)last;
}

/*
 * strstr - Find first occurrence of needle in haystack
 *
 * Returns pointer to start of match, or NULL if not found.
 */
char* strstr(const char* haystack, const char* needle) {
    size_t needle_len;
    
    if (*needle == '\0') {
        return (char*)haystack;
    }
    
    needle_len = strlen(needle);
    
    while (*haystack != '\0') {
        if (strncmp(haystack, needle, needle_len) == 0) {
            return (char*)haystack;
        }
        haystack++;
    }
    
    return NULL;
}

/*
 * itoa - Convert integer to string
 *
 * Supports bases 2-36 (binary, octal, decimal, hex, etc.)
 * Handles negative numbers for base 10 only.
 *
 * Example:
 *   char buf[32];
 *   itoa(255, buf, 16);  // buf = "ff"
 *   itoa(-42, buf, 10);  // buf = "-42"
 */
char* itoa(int value, char* str, int base) {
    char* ptr = str;
    char* low;
    int negative = 0;
    
    /* Validate base */
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    /* Handle 0 explicitly */
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    /* Handle negative numbers (only for base 10) */
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    /* Convert digits in reverse order */
    while (value != 0) {
        int digit = value % base;
        *ptr++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        value /= base;
    }
    
    /* Add negative sign if needed */
    if (negative) {
        *ptr++ = '-';
    }
    
    *ptr = '\0';
    
    /* Reverse the string */
    low = str;
    ptr--;  /* Point to last character (before null) */
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return str;
}

/*
 * atoi - Convert string to integer
 *
 * Skips leading whitespace, handles optional +/- sign.
 * Stops at first non-digit character.
 *
 * Examples:
 *   atoi("42")     => 42
 *   atoi("  -123") => -123
 *   atoi("12abc")  => 12
 */
int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    /* Skip whitespace */
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }
    
    /* Handle sign */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    /* Convert digits */
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}
