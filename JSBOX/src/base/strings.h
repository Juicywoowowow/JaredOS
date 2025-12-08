/*
 * JSBOX - JavaScript Engine
 * 
 * Base: String Utilities
 * 
 * Helper functions for string manipulation.
 */

#ifndef JSBOX_STRINGS_H
#define JSBOX_STRINGS_H

#include <stddef.h>
#include <stdbool.h>
#include "memory.h"

/* ============================================================================
 * String Builder
 * ============================================================================ */

typedef struct JSBox_StringBuilder {
    char* data;
    size_t length;
    size_t capacity;
} JSBox_StringBuilder;

/* Create a new string builder */
JSBox_StringBuilder* jsbox_sb_create(void);

/* Create with initial capacity */
JSBox_StringBuilder* jsbox_sb_create_sized(size_t capacity);

/* Destroy string builder */
void jsbox_sb_destroy(JSBox_StringBuilder* sb);

/* Append a character */
void jsbox_sb_append_char(JSBox_StringBuilder* sb, char c);

/* Append a string */
void jsbox_sb_append(JSBox_StringBuilder* sb, const char* str);

/* Append n characters */
void jsbox_sb_append_n(JSBox_StringBuilder* sb, const char* str, size_t n);

/* Append formatted string */
void jsbox_sb_appendf(JSBox_StringBuilder* sb, const char* fmt, ...);

/* Get the built string (caller must NOT free) */
const char* jsbox_sb_str(const JSBox_StringBuilder* sb);

/* Get a copy of the string (caller must free) */
char* jsbox_sb_to_string(const JSBox_StringBuilder* sb);

/* Clear the string builder */
void jsbox_sb_clear(JSBox_StringBuilder* sb);

/* Get length */
size_t jsbox_sb_length(const JSBox_StringBuilder* sb);

/* ============================================================================
 * String Utilities
 * ============================================================================ */

/* Check if string starts with prefix */
bool jsbox_str_starts_with(const char* str, const char* prefix);

/* Check if string ends with suffix */
bool jsbox_str_ends_with(const char* str, const char* suffix);

/* Check if character is digit */
bool jsbox_is_digit(char c);

/* Check if character is alpha */
bool jsbox_is_alpha(char c);

/* Check if character is alphanumeric */
bool jsbox_is_alnum(char c);

/* Check if character is whitespace */
bool jsbox_is_space(char c);

/* Check if character is valid JS identifier start */
bool jsbox_is_ident_start(char c);

/* Check if character is valid JS identifier part */
bool jsbox_is_ident_part(char c);

/* Escape string for display (allocates) */
char* jsbox_str_escape(const char* str);

/* Get substring (allocates) */
char* jsbox_str_substr(const char* str, size_t start, size_t len);

#endif /* JSBOX_STRINGS_H */
