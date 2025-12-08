/*
 * JSBOX - JavaScript Engine
 * 
 * Base: String Utilities Implementation
 */

#include "strings.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define SB_INITIAL_CAPACITY 64

/* ============================================================================
 * String Builder Implementation
 * ============================================================================ */

JSBox_StringBuilder* jsbox_sb_create(void) {
    return jsbox_sb_create_sized(SB_INITIAL_CAPACITY);
}

JSBox_StringBuilder* jsbox_sb_create_sized(size_t capacity) {
    JSBox_StringBuilder* sb = (JSBox_StringBuilder*)jsbox_malloc(sizeof(JSBox_StringBuilder));
    sb->data = (char*)jsbox_malloc(capacity);
    sb->data[0] = '\0';
    sb->length = 0;
    sb->capacity = capacity;
    return sb;
}

void jsbox_sb_destroy(JSBox_StringBuilder* sb) {
    if (sb) {
        jsbox_free(sb->data);
        jsbox_free(sb);
    }
}

static void sb_grow(JSBox_StringBuilder* sb, size_t needed) {
    if (sb->length + needed + 1 > sb->capacity) {
        size_t new_cap = sb->capacity * 2;
        while (new_cap < sb->length + needed + 1) {
            new_cap *= 2;
        }
        sb->data = (char*)jsbox_realloc(sb->data, new_cap);
        sb->capacity = new_cap;
    }
}

void jsbox_sb_append_char(JSBox_StringBuilder* sb, char c) {
    sb_grow(sb, 1);
    sb->data[sb->length++] = c;
    sb->data[sb->length] = '\0';
}

void jsbox_sb_append(JSBox_StringBuilder* sb, const char* str) {
    if (!str) return;
    size_t len = strlen(str);
    sb_grow(sb, len);
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

void jsbox_sb_append_n(JSBox_StringBuilder* sb, const char* str, size_t n) {
    if (!str || n == 0) return;
    sb_grow(sb, n);
    memcpy(sb->data + sb->length, str, n);
    sb->length += n;
    sb->data[sb->length] = '\0';
}

void jsbox_sb_appendf(JSBox_StringBuilder* sb, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    /* First, determine needed size */
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    
    if (needed < 0) {
        va_end(args);
        return;
    }
    
    sb_grow(sb, (size_t)needed);
    vsnprintf(sb->data + sb->length, (size_t)needed + 1, fmt, args);
    sb->length += (size_t)needed;
    
    va_end(args);
}

const char* jsbox_sb_str(const JSBox_StringBuilder* sb) {
    return sb->data;
}

char* jsbox_sb_to_string(const JSBox_StringBuilder* sb) {
    return jsbox_strdup(sb->data);
}

void jsbox_sb_clear(JSBox_StringBuilder* sb) {
    sb->length = 0;
    sb->data[0] = '\0';
}

size_t jsbox_sb_length(const JSBox_StringBuilder* sb) {
    return sb->length;
}

/* ============================================================================
 * String Utilities Implementation
 * ============================================================================ */

bool jsbox_str_starts_with(const char* str, const char* prefix) {
    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
}

bool jsbox_str_ends_with(const char* str, const char* suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool jsbox_is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool jsbox_is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool jsbox_is_alnum(char c) {
    return jsbox_is_alpha(c) || jsbox_is_digit(c);
}

bool jsbox_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

bool jsbox_is_ident_start(char c) {
    return jsbox_is_alpha(c) || c == '_' || c == '$';
}

bool jsbox_is_ident_part(char c) {
    return jsbox_is_alnum(c) || c == '_' || c == '$';
}

char* jsbox_str_escape(const char* str) {
    if (!str) return jsbox_strdup("(null)");
    
    JSBox_StringBuilder* sb = jsbox_sb_create();
    
    while (*str) {
        switch (*str) {
            case '\n': jsbox_sb_append(sb, "\\n"); break;
            case '\r': jsbox_sb_append(sb, "\\r"); break;
            case '\t': jsbox_sb_append(sb, "\\t"); break;
            case '\\': jsbox_sb_append(sb, "\\\\"); break;
            case '"':  jsbox_sb_append(sb, "\\\""); break;
            case '\'': jsbox_sb_append(sb, "\\'"); break;
            default:
                if (*str < 32 || *str > 126) {
                    jsbox_sb_appendf(sb, "\\x%02x", (unsigned char)*str);
                } else {
                    jsbox_sb_append_char(sb, *str);
                }
                break;
        }
        str++;
    }
    
    char* result = jsbox_sb_to_string(sb);
    jsbox_sb_destroy(sb);
    return result;
}

char* jsbox_str_substr(const char* str, size_t start, size_t len) {
    size_t str_len = strlen(str);
    if (start >= str_len) return jsbox_strdup("");
    if (start + len > str_len) len = str_len - start;
    
    char* result = (char*)jsbox_malloc(len + 1);
    memcpy(result, str + start, len);
    result[len] = '\0';
    return result;
}
