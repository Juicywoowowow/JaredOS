/*
 * =============================================================================
 * SharkOS String Utilities Header (string.h)
 * =============================================================================
 * String manipulation functions for the kernel.
 * =============================================================================
 */

#ifndef STRING_H
#define STRING_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * strlen - Get length of string (not including null terminator)
 * ---------------------------------------------------------------------------- */
size_t strlen(const char* str);

/* ----------------------------------------------------------------------------
 * strcmp - Compare two strings
 * @return: 0 if equal, <0 if s1<s2, >0 if s1>s2
 * ---------------------------------------------------------------------------- */
int strcmp(const char* s1, const char* s2);

/* ----------------------------------------------------------------------------
 * strncmp - Compare first n characters of two strings
 * ---------------------------------------------------------------------------- */
int strncmp(const char* s1, const char* s2, size_t n);

/* ----------------------------------------------------------------------------
 * strcpy - Copy string from src to dest
 * WARNING: No bounds checking! Make sure dest is large enough.
 * ---------------------------------------------------------------------------- */
char* strcpy(char* dest, const char* src);

/* ----------------------------------------------------------------------------
 * strncpy - Copy up to n characters from src to dest
 * ---------------------------------------------------------------------------- */
char* strncpy(char* dest, const char* src, size_t n);

/* ----------------------------------------------------------------------------
 * strcat - Append src to end of dest
 * WARNING: No bounds checking! Make sure dest is large enough.
 * ---------------------------------------------------------------------------- */
char* strcat(char* dest, const char* src);

/* ----------------------------------------------------------------------------
 * strchr - Find first occurrence of character in string
 * @return: Pointer to character, or NULL if not found
 * ---------------------------------------------------------------------------- */
char* strchr(const char* str, int c);

/* ----------------------------------------------------------------------------
 * strrchr - Find last occurrence of character in string
 * @return: Pointer to character, or NULL if not found
 * ---------------------------------------------------------------------------- */
char* strrchr(const char* str, int c);

/* ----------------------------------------------------------------------------
 * strstr - Find first occurrence of substring in string
 * @return: Pointer to substring, or NULL if not found
 * ---------------------------------------------------------------------------- */
char* strstr(const char* haystack, const char* needle);

/* ----------------------------------------------------------------------------
 * itoa - Convert integer to string (custom implementation)
 * @param value: Integer to convert
 * @param str: Buffer to store result (must be at least 12 chars for int32)
 * @param base: Number base (2-36, commonly 10 or 16)
 * @return: Pointer to str
 * ---------------------------------------------------------------------------- */
char* itoa(int value, char* str, int base);

/* ----------------------------------------------------------------------------
 * atoi - Convert string to integer
 * Handles optional leading sign (+/-) and whitespace
 * ---------------------------------------------------------------------------- */
int atoi(const char* str);

#endif /* STRING_H */
