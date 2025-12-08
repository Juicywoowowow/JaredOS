/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Hash Map (String -> Pointer)
 * 
 * Open addressing hash table with FNV-1a hashing.
 * Used for object properties, variable scopes, etc.
 */

#ifndef JSBOX_HASHMAP_H
#define JSBOX_HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Hash Map Types
 * ============================================================================ */

typedef struct JSBox_HashEntry {
    char* key;
    void* value;
    uint32_t hash;
    bool occupied;
    bool deleted;
} JSBox_HashEntry;

typedef struct JSBox_HashMap {
    JSBox_HashEntry* entries;
    size_t capacity;
    size_t count;
    size_t deleted_count;
} JSBox_HashMap;

/* Iteration callback */
typedef void (*JSBox_HashMapIterFn)(const char* key, void* value, void* ctx);

/* ============================================================================
 * Hash Map API
 * ============================================================================ */

/* Create a new hash map */
JSBox_HashMap* jsbox_hashmap_create(void);

/* Create with initial capacity */
JSBox_HashMap* jsbox_hashmap_create_sized(size_t capacity);

/* Destroy hash map (does NOT free values) */
void jsbox_hashmap_destroy(JSBox_HashMap* map);

/* Destroy hash map and free all values with given free function */
void jsbox_hashmap_destroy_full(JSBox_HashMap* map, void (*free_fn)(void*));

/* Set a key-value pair (key is copied) */
void jsbox_hashmap_set(JSBox_HashMap* map, const char* key, void* value);

/* Get value by key (returns NULL if not found) */
void* jsbox_hashmap_get(const JSBox_HashMap* map, const char* key);

/* Check if key exists */
bool jsbox_hashmap_has(const JSBox_HashMap* map, const char* key);

/* Delete a key (returns old value, or NULL if not found) */
void* jsbox_hashmap_delete(JSBox_HashMap* map, const char* key);

/* Get number of entries */
size_t jsbox_hashmap_count(const JSBox_HashMap* map);

/* Clear all entries */
void jsbox_hashmap_clear(JSBox_HashMap* map);

/* Iterate over all entries */
void jsbox_hashmap_iter(const JSBox_HashMap* map, JSBox_HashMapIterFn fn, void* ctx);

/* ============================================================================
 * Hash Function
 * ============================================================================ */

/* FNV-1a hash */
uint32_t jsbox_hash_string(const char* str);
uint32_t jsbox_hash_bytes(const void* data, size_t len);

#endif /* JSBOX_HASHMAP_H */
