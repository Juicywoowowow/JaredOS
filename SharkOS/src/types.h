/*
 * =============================================================================
 * SharkOS Type Definitions (types.h)
 * =============================================================================
 * Standard integer types for kernel development.
 * We can't use <stdint.h> because we're freestanding (no standard library).
 *
 * DEBUGGING TIPS:
 *   - If you see weird values, check if you're using the right type size
 *   - uint8_t should be 1 byte, uint16_t = 2, uint32_t = 4
 *   - Use sizeof() at compile time to verify: _Static_assert(sizeof(uint32_t) == 4, "");
 * =============================================================================
 */

#ifndef TYPES_H
#define TYPES_H

/* ----------------------------------------------------------------------------
 * Unsigned integer types
 * These match the sizes expected by x86 hardware
 * ---------------------------------------------------------------------------- */
typedef unsigned char       uint8_t;    /* 8-bit unsigned (0-255) */
typedef unsigned short      uint16_t;   /* 16-bit unsigned (0-65535) */
typedef unsigned int        uint32_t;   /* 32-bit unsigned (0-4294967295) */
typedef unsigned long long  uint64_t;   /* 64-bit unsigned */

/* ----------------------------------------------------------------------------
 * Signed integer types
 * For when you need negative numbers
 * ---------------------------------------------------------------------------- */
typedef signed char         int8_t;     /* 8-bit signed (-128 to 127) */
typedef signed short        int16_t;    /* 16-bit signed */
typedef signed int          int32_t;    /* 32-bit signed */
typedef signed long long    int64_t;    /* 64-bit signed */

/* ----------------------------------------------------------------------------
 * Size types
 * Used for memory sizes and array indices
 * ---------------------------------------------------------------------------- */
typedef uint32_t            size_t;     /* Size of objects in memory */
typedef int32_t             ssize_t;    /* Signed size (for error returns) */

/* ----------------------------------------------------------------------------
 * Boolean type
 * Note: In C89, we don't have _Bool, so we use int
 * ---------------------------------------------------------------------------- */
typedef int                 bool;
#define true                1
#define false               0

/* ----------------------------------------------------------------------------
 * Null pointer
 * ---------------------------------------------------------------------------- */
#define NULL                ((void*)0)

#endif /* TYPES_H */
