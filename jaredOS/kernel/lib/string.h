/**
 * jaredOS - String Functions Header
 */

#ifndef STRING_H
#define STRING_H

#include "../types.h"

/* String length */
size_t strlen(const char *str);

/* String compare */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

/* String copy */
char* strcpy(char *dest, const char *src);
char* strncpy(char *dest, const char *src, size_t n);

/* String concatenate */
char* strcat(char *dest, const char *src);

/* Find character in string */
char* strchr(const char *str, int c);
char* strrchr(const char *str, int c);

/* Memory operations */
void* memset(void *ptr, int value, size_t num);
void* memcpy(void *dest, const void *src, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void* memmove(void *dest, const void *src, size_t num);

#endif /* STRING_H */
