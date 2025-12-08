/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Hash Map Implementation
 */

#include "hashmap.h"
#include "memory.h"
#include <string.h>

#define HASHMAP_INITIAL_CAPACITY 16
#define HASHMAP_LOAD_FACTOR 0.75

/* ============================================================================
 * Hash Functions
 * ============================================================================ */

/* FNV-1a constants */
#define FNV_OFFSET 2166136261u
#define FNV_PRIME  16777619u

uint32_t jsbox_hash_bytes(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t hash = FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

uint32_t jsbox_hash_string(const char* str) {
    uint32_t hash = FNV_OFFSET;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= FNV_PRIME;
    }
    return hash;
}

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static void hashmap_resize(JSBox_HashMap* map, size_t new_capacity) {
    JSBox_HashEntry* old_entries = map->entries;
    size_t old_capacity = map->capacity;
    
    map->entries = (JSBox_HashEntry*)jsbox_calloc(new_capacity, sizeof(JSBox_HashEntry));
    map->capacity = new_capacity;
    map->count = 0;
    map->deleted_count = 0;
    
    /* Rehash all entries */
    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].occupied && !old_entries[i].deleted) {
            jsbox_hashmap_set(map, old_entries[i].key, old_entries[i].value);
            jsbox_free(old_entries[i].key);
        }
    }
    
    jsbox_free(old_entries);
}

static JSBox_HashEntry* hashmap_find_entry(JSBox_HashEntry* entries, size_t capacity, 
                                            const char* key, uint32_t hash) {
    size_t index = hash & (capacity - 1);
    JSBox_HashEntry* tombstone = NULL;
    
    for (;;) {
        JSBox_HashEntry* entry = &entries[index];
        
        if (!entry->occupied) {
            /* Empty slot */
            return tombstone ? tombstone : entry;
        }
        
        if (entry->deleted) {
            /* Tombstone - remember first one */
            if (!tombstone) tombstone = entry;
        } else if (entry->hash == hash && strcmp(entry->key, key) == 0) {
            /* Found the key */
            return entry;
        }
        
        index = (index + 1) & (capacity - 1);
    }
}

/* ============================================================================
 * Hash Map API Implementation
 * ============================================================================ */

JSBox_HashMap* jsbox_hashmap_create(void) {
    return jsbox_hashmap_create_sized(HASHMAP_INITIAL_CAPACITY);
}

JSBox_HashMap* jsbox_hashmap_create_sized(size_t capacity) {
    /* Round up to power of 2 */
    size_t actual = HASHMAP_INITIAL_CAPACITY;
    while (actual < capacity) {
        actual *= 2;
    }
    
    JSBox_HashMap* map = (JSBox_HashMap*)jsbox_malloc(sizeof(JSBox_HashMap));
    map->entries = (JSBox_HashEntry*)jsbox_calloc(actual, sizeof(JSBox_HashEntry));
    map->capacity = actual;
    map->count = 0;
    map->deleted_count = 0;
    
    return map;
}

void jsbox_hashmap_destroy(JSBox_HashMap* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied && !map->entries[i].deleted) {
            jsbox_free(map->entries[i].key);
        }
    }
    
    jsbox_free(map->entries);
    jsbox_free(map);
}

void jsbox_hashmap_destroy_full(JSBox_HashMap* map, void (*free_fn)(void*)) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied && !map->entries[i].deleted) {
            jsbox_free(map->entries[i].key);
            if (free_fn) {
                free_fn(map->entries[i].value);
            }
        }
    }
    
    jsbox_free(map->entries);
    jsbox_free(map);
}

void jsbox_hashmap_set(JSBox_HashMap* map, const char* key, void* value) {
    /* Check if we need to grow */
    if ((map->count + map->deleted_count + 1) > map->capacity * HASHMAP_LOAD_FACTOR) {
        hashmap_resize(map, map->capacity * 2);
    }
    
    uint32_t hash = jsbox_hash_string(key);
    JSBox_HashEntry* entry = hashmap_find_entry(map->entries, map->capacity, key, hash);
    
    bool is_new = !entry->occupied || entry->deleted;
    
    if (is_new) {
        if (entry->deleted) {
            map->deleted_count--;
        }
        entry->key = jsbox_strdup(key);
        map->count++;
    } else {
        /* Key exists, free old key (we'll set same key) */
        jsbox_free(entry->key);
        entry->key = jsbox_strdup(key);
    }
    
    entry->value = value;
    entry->hash = hash;
    entry->occupied = true;
    entry->deleted = false;
}

void* jsbox_hashmap_get(const JSBox_HashMap* map, const char* key) {
    if (map->count == 0) return NULL;
    
    uint32_t hash = jsbox_hash_string(key);
    JSBox_HashEntry* entry = hashmap_find_entry(map->entries, map->capacity, key, hash);
    
    if (!entry->occupied || entry->deleted) {
        return NULL;
    }
    
    return entry->value;
}

bool jsbox_hashmap_has(const JSBox_HashMap* map, const char* key) {
    if (map->count == 0) return false;
    
    uint32_t hash = jsbox_hash_string(key);
    JSBox_HashEntry* entry = hashmap_find_entry(map->entries, map->capacity, key, hash);
    
    return entry->occupied && !entry->deleted;
}

void* jsbox_hashmap_delete(JSBox_HashMap* map, const char* key) {
    if (map->count == 0) return NULL;
    
    uint32_t hash = jsbox_hash_string(key);
    JSBox_HashEntry* entry = hashmap_find_entry(map->entries, map->capacity, key, hash);
    
    if (!entry->occupied || entry->deleted) {
        return NULL;
    }
    
    void* value = entry->value;
    jsbox_free(entry->key);
    entry->key = NULL;
    entry->value = NULL;
    entry->deleted = true;
    map->count--;
    map->deleted_count++;
    
    return value;
}

size_t jsbox_hashmap_count(const JSBox_HashMap* map) {
    return map->count;
}

void jsbox_hashmap_clear(JSBox_HashMap* map) {
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied && !map->entries[i].deleted) {
            jsbox_free(map->entries[i].key);
        }
        map->entries[i].occupied = false;
        map->entries[i].deleted = false;
    }
    map->count = 0;
    map->deleted_count = 0;
}

void jsbox_hashmap_iter(const JSBox_HashMap* map, JSBox_HashMapIterFn fn, void* ctx) {
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].occupied && !map->entries[i].deleted) {
            fn(map->entries[i].key, map->entries[i].value, ctx);
        }
    }
}
