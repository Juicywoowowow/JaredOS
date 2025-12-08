// CU Standard Library - Portable Runtime
// This file provides platform-independent utilities

#ifndef CU_STD_H
#define CU_STD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Memory Safety Traps (WASM-style)
// ============================================================================

// Internal trap function - prints error and aborts
static inline void cu_trap_impl(const char* msg, const char* file, int line) {
    fprintf(stderr, "CU TRAP: %s\n  at %s:%d\n", msg, file, line);
    abort();
}

// Macro for statement context
#define CU_TRAP(msg, file, line) cu_trap_impl(msg, file, line)

// Null pointer check - can be used in expressions via comma operator
static inline void cu_check_null_impl(const void* ptr, const char* file, int line) {
    if (ptr == NULL) {
        cu_trap_impl("null pointer dereference", file, line);
    }
}
#define CU_CHECK_NULL(ptr, file, line) cu_check_null_impl((const void*)(ptr), file, line)

// Array bounds check
static inline void cu_check_bounds_impl(uint64_t idx, uint64_t len, const char* file, int line) {
    if (idx >= len) {
        cu_trap_impl("array index out of bounds", file, line);
    }
}
#define CU_CHECK_BOUNDS(idx, len, file, line) cu_check_bounds_impl((uint64_t)(idx), (uint64_t)(len), file, line)

// Division by zero check - evaluates to nothing (used in comma expr)
static inline void cu_check_div_zero_impl(int64_t divisor, const char* file, int line) {
    if (divisor == 0) {
        cu_trap_impl("division by zero", file, line);
    }
}
#define CU_CHECK_DIV_ZERO(divisor, file, line) cu_check_div_zero_impl((int64_t)(divisor), file, line)

// Integer overflow check (for adding)
#define CU_CHECK_ADD_OVERFLOW_I32(a, b, file, line) do { \
    if (((b) > 0 && (a) > INT32_MAX - (b)) || \
        ((b) < 0 && (a) < INT32_MIN - (b))) { \
        CU_TRAP("integer overflow (addition)", file, line); \
    } \
} while(0)

// Integer overflow check (for multiplying)
#define CU_CHECK_MUL_OVERFLOW_I32(a, b, file, line) do { \
    if ((a) != 0 && (b) != 0) { \
        if ((a) > 0) { \
            if ((b) > 0) { \
                if ((a) > INT32_MAX / (b)) CU_TRAP("integer overflow (multiplication)", file, line); \
            } else { \
                if ((b) < INT32_MIN / (a)) CU_TRAP("integer overflow (multiplication)", file, line); \
            } \
        } else { \
            if ((b) > 0) { \
                if ((a) < INT32_MIN / (b)) CU_TRAP("integer overflow (multiplication)", file, line); \
            } else { \
                if ((a) != 0 && (b) < INT32_MAX / (a)) CU_TRAP("integer overflow (multiplication)", file, line); \
            } \
        } \
    } \
} while(0)

// ============================================================================
// Platform Detection
// ============================================================================

// Operating Systems
#if defined(__linux__)
    #define PLATFORM_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS 1
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #define PLATFORM_BSD 1
#elif defined(__ANDROID__)
    #define PLATFORM_ANDROID 1
#elif defined(__EMSCRIPTEN__)
    #define PLATFORM_WASM 1
#endif

// Architectures
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X86_64 1
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_X86_32 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define ARCH_ARM32 1
#elif defined(__riscv)
    #define ARCH_RISCV 1
#elif defined(__wasm__) || defined(__wasm32__)
    #define ARCH_WASM 1
#endif

// Endianness Detection
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define CU_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define CU_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
    #define CU_BIG_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN)
    #define CU_LITTLE_ENDIAN 1
#else
    // Default to little endian (most common)
    #define CU_LITTLE_ENDIAN 1
#endif

// ============================================================================
// Endianness Conversion Functions
// ============================================================================

static inline uint16_t cu_bswap16(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint32_t cu_bswap32(uint32_t x) {
    return ((x & 0xFF000000u) >> 24) |
           ((x & 0x00FF0000u) >> 8)  |
           ((x & 0x0000FF00u) << 8)  |
           ((x & 0x000000FFu) << 24);
}

static inline uint64_t cu_bswap64(uint64_t x) {
    return ((x & 0xFF00000000000000ull) >> 56) |
           ((x & 0x00FF000000000000ull) >> 40) |
           ((x & 0x0000FF0000000000ull) >> 24) |
           ((x & 0x000000FF00000000ull) >> 8)  |
           ((x & 0x00000000FF000000ull) << 8)  |
           ((x & 0x0000000000FF0000ull) << 24) |
           ((x & 0x000000000000FF00ull) << 40) |
           ((x & 0x00000000000000FFull) << 56);
}

// Big Endian Conversions
#ifdef CU_BIG_ENDIAN
    #define cu_be16(x) (x)
    #define cu_be32(x) (x)
    #define cu_be64(x) (x)
    #define cu_le16(x) cu_bswap16(x)
    #define cu_le32(x) cu_bswap32(x)
    #define cu_le64(x) cu_bswap64(x)
#else
    #define cu_be16(x) cu_bswap16(x)
    #define cu_be32(x) cu_bswap32(x)
    #define cu_be64(x) cu_bswap64(x)
    #define cu_le16(x) (x)
    #define cu_le32(x) (x)
    #define cu_le64(x) (x)
#endif

// ============================================================================
// Memory Operations (No libc dependency)
// ============================================================================

static inline void* cu_memcpy(void* dest, const void* src, uint64_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static inline void* cu_memset(void* ptr, uint8_t value, uint64_t n) {
    uint8_t* p = (uint8_t*)ptr;
    for (uint64_t i = 0; i < n; i++) {
        p[i] = value;
    }
    return ptr;
}

static inline int32_t cu_memcmp(const void* a, const void* b, uint64_t n) {
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    for (uint64_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return pa[i] - pb[i];
        }
    }
    return 0;
}

// ============================================================================
// String Operations
// ============================================================================

static inline uint64_t cu_strlen(const char* str) {
    uint64_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

static inline int32_t cu_strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a - *b;
}

static inline char* cu_strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

static inline char* cu_strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != '\0');
    return dest;
}

// ============================================================================
// Alignment Utilities
// ============================================================================

// Check if pointer is aligned
static inline bool cu_is_aligned(const void* ptr, uint64_t alignment) {
    return ((uint64_t)ptr & (alignment - 1)) == 0;
}

// Align pointer up to alignment
static inline void* cu_align_up(void* ptr, uint64_t alignment) {
    uint64_t addr = (uint64_t)ptr;
    uint64_t mask = alignment - 1;
    return (void*)((addr + mask) & ~mask);
}

// Align pointer down to alignment
static inline void* cu_align_down(void* ptr, uint64_t alignment) {
    uint64_t addr = (uint64_t)ptr;
    uint64_t mask = alignment - 1;
    return (void*)(addr & ~mask);
}

// ============================================================================
// Math Utilities
// ============================================================================

static inline int32_t cu_abs_i32(int32_t x) {
    return x < 0 ? -x : x;
}

static inline int64_t cu_abs_i64(int64_t x) {
    return x < 0 ? -x : x;
}

static inline int32_t cu_min_i32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

static inline int32_t cu_max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

static inline uint32_t cu_min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

static inline uint32_t cu_max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

// ============================================================================
// Bit Manipulation
// ============================================================================

static inline uint32_t cu_popcount32(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x3Fu;
}

static inline uint32_t cu_clz32(uint32_t x) {
    if (x == 0) return 32;
    uint32_t n = 0;
    if (x <= 0x0000FFFFu) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFFu) { n += 8;  x <<= 8;  }
    if (x <= 0x0FFFFFFFu) { n += 4;  x <<= 4;  }
    if (x <= 0x3FFFFFFFu) { n += 2;  x <<= 2;  }
    if (x <= 0x7FFFFFFFu) { n += 1; }
    return n;
}

static inline uint32_t cu_ctz32(uint32_t x) {
    if (x == 0) return 32;
    uint32_t n = 0;
    if ((x & 0x0000FFFFu) == 0) { n += 16; x >>= 16; }
    if ((x & 0x000000FFu) == 0) { n += 8;  x >>= 8;  }
    if ((x & 0x0000000Fu) == 0) { n += 4;  x >>= 4;  }
    if ((x & 0x00000003u) == 0) { n += 2;  x >>= 2;  }
    if ((x & 0x00000001u) == 0) { n += 1; }
    return n;
}

// ============================================================================
// Platform Information
// ============================================================================

typedef struct {
    uint64_t pointer_size;
    uint64_t pointer_align;
    bool is_little_endian;
    bool is_big_endian;
    const char* platform_name;
    const char* arch_name;
} cu_platform_info;

static inline cu_platform_info cu_get_platform_info(void) {
    cu_platform_info info;
    info.pointer_size = sizeof(void*);
    info.pointer_align = _Alignof(void*);
    
#ifdef CU_LITTLE_ENDIAN
    info.is_little_endian = true;
    info.is_big_endian = false;
#else
    info.is_little_endian = false;
    info.is_big_endian = true;
#endif

static inline void cu_get_platform_info_ptr(cu_platform_info* out) {
    if (out) *out = cu_get_platform_info();
}

#ifdef PLATFORM_LINUX
    info.platform_name = "Linux";
#elif defined(PLATFORM_WINDOWS)
    info.platform_name = "Windows";
#elif defined(PLATFORM_MACOS)
    info.platform_name = "macOS";
#elif defined(PLATFORM_BSD)
    info.platform_name = "BSD";
#elif defined(PLATFORM_WASM)
    info.platform_name = "WebAssembly";
#else
    info.platform_name = "Unknown";
#endif

#ifdef ARCH_X86_64
    info.arch_name = "x86_64";
#elif defined(ARCH_ARM64)
    info.arch_name = "ARM64";
#elif defined(ARCH_ARM32)
    info.arch_name = "ARM32";
#elif defined(ARCH_RISCV)
    info.arch_name = "RISC-V";
#elif defined(ARCH_WASM)
    info.arch_name = "WebAssembly";
#else
    info.arch_name = "Unknown";
#endif

    return info;
}

#endif // CU_STD_H
