/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Dynamic Vector (Type-Safe Generic Array)
 * 
 * Usage:
 *   JSBOX_VEC_DEFINE(int, IntVec)
 *   IntVec* vec = intvec_create();
 *   intvec_push(vec, 42);
 *   int val = intvec_get(vec, 0);
 */

#ifndef JSBOX_VECTOR_H
#define JSBOX_VECTOR_H

#include <stddef.h>
#include <stdbool.h>
#include "memory.h"

/* ============================================================================
 * Generic Vector Macro
 * ============================================================================ */

#define JSBOX_VEC_DEFINE(T, Name)                                              \
    typedef struct {                                                           \
        T* data;                                                               \
        size_t length;                                                         \
        size_t capacity;                                                       \
    } Name;                                                                    \
                                                                               \
    static inline Name* Name##_create(void) {                                  \
        Name* vec = (Name*)jsbox_malloc(sizeof(Name));                         \
        vec->data = NULL;                                                      \
        vec->length = 0;                                                       \
        vec->capacity = 0;                                                     \
        return vec;                                                            \
    }                                                                          \
                                                                               \
    static inline Name* Name##_with_capacity(size_t cap) {                     \
        Name* vec = (Name*)jsbox_malloc(sizeof(Name));                         \
        vec->data = (T*)jsbox_malloc(sizeof(T) * cap);                         \
        vec->length = 0;                                                       \
        vec->capacity = cap;                                                   \
        return vec;                                                            \
    }                                                                          \
                                                                               \
    static inline void Name##_destroy(Name* vec) {                             \
        if (vec) {                                                             \
            jsbox_free(vec->data);                                             \
            jsbox_free(vec);                                                   \
        }                                                                      \
    }                                                                          \
                                                                               \
    static inline void Name##_grow(Name* vec) {                                \
        size_t new_cap = vec->capacity == 0 ? 8 : vec->capacity * 2;           \
        vec->data = (T*)jsbox_realloc(vec->data, sizeof(T) * new_cap);         \
        vec->capacity = new_cap;                                               \
    }                                                                          \
                                                                               \
    static inline void Name##_push(Name* vec, T value) {                       \
        if (vec->length >= vec->capacity) {                                    \
            Name##_grow(vec);                                                  \
        }                                                                      \
        vec->data[vec->length++] = value;                                      \
    }                                                                          \
                                                                               \
    static inline T Name##_pop(Name* vec) {                                    \
        return vec->data[--vec->length];                                       \
    }                                                                          \
                                                                               \
    static inline T Name##_get(const Name* vec, size_t index) {                \
        return vec->data[index];                                               \
    }                                                                          \
                                                                               \
    static inline void Name##_set(Name* vec, size_t index, T value) {          \
        vec->data[index] = value;                                              \
    }                                                                          \
                                                                               \
    static inline T* Name##_at(Name* vec, size_t index) {                      \
        return &vec->data[index];                                              \
    }                                                                          \
                                                                               \
    static inline size_t Name##_length(const Name* vec) {                      \
        return vec->length;                                                    \
    }                                                                          \
                                                                               \
    static inline bool Name##_empty(const Name* vec) {                         \
        return vec->length == 0;                                               \
    }                                                                          \
                                                                               \
    static inline void Name##_clear(Name* vec) {                               \
        vec->length = 0;                                                       \
    }                                                                          \
                                                                               \
    static inline T Name##_last(const Name* vec) {                             \
        return vec->data[vec->length - 1];                                     \
    }

/* ============================================================================
 * Common Vector Types
 * ============================================================================ */

/* Forward declare common types for headers */
struct JSBox_Token;
struct JSBox_ASTNode;
struct JSBox_Diagnostic;

#endif /* JSBOX_VECTOR_H */
