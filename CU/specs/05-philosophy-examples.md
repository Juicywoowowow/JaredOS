# CU Philosophy & Examples

## Core Philosophy

### 1. Simplicity Above All

**Principle**: If a feature adds complexity, it doesn't belong in CU.

CU is intentionally minimal. Every feature must justify its existence by solving a real problem without adding cognitive overhead.

#### What CU Has:
- Basic types (integers, floats, bool)
- Structs, unions, enums
- Pointers and arrays
- Functions
- Minimal control flow

#### What CU Doesn't Have:
- ❌ Classes and inheritance
- ❌ Generics/templates
- ❌ Exceptions
- ❌ Garbage collection
- ❌ Dynamic dispatch
- ❌ Operator overloading
- ❌ Implicit conversions
- ❌ Macros (beyond minimal preprocessor)

**Example - Simple is Better:**

```cu
// CU way: Explicit and simple
fn add(i32 a, i32 b) -> i32 {
    return a + b;
}

// NOT the CU way: Over-engineered
// template<typename T>
// class Adder {
//     T operator()(T a, T b) { return a + b; }
// };
```

### 2. Portability is Non-Negotiable

**Principle**: Write once, compile anywhere (where C/C++ works).

CU code should behave identically across platforms. Platform-specific code must be explicitly marked.

**Example - Portable Code:**

```cu
// Portable: Uses fixed-size types
fn hash(u8* data, u64 len) -> u32 {
    u32 hash = 2166136261u;
    for (u64 i = 0; i < len; i++) {
        hash = hash ^ data[i];
        hash = hash * 16777619u;
    }
    return hash;
}

// Platform-specific: Clearly marked
#if defined(PLATFORM_LINUX)
    fn get_page_size() -> u64 {
        return 4096;
    }
#elif defined(PLATFORM_WINDOWS)
    fn get_page_size() -> u64 {
        return 4096;  // Typically, but can vary
    }
#endif
```

### 3. Explicitness Over Convenience

**Principle**: No surprises. The code does exactly what it says.

CU avoids implicit behavior. Every operation is visible in the source code.

**Example - Explicit Operations:**

```cu
// Explicit casting
i32 x = 42;
f32 f = cast(f32) x;  // Must explicitly cast

// Explicit memory management
i32* ptr = cast(i32*) malloc(sizeof(i32) * 100);
// ... use ptr ...
free(ptr);  // Must explicitly free

// Explicit error handling
fn divide(i32 a, i32 b, i32* result) -> bool {
    if (b == 0) {
        return false;  // Explicit error return
    }
    *result = a / b;
    return true;
}
```

### 4. Zero-Cost Abstractions

**Principle**: You don't pay for what you don't use. Abstractions compile to optimal code.

CU's abstractions should have zero runtime overhead compared to hand-written assembly.

**Example - Zero Cost:**

```cu
// High-level abstraction
struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
}

fn Vec3_add(Vec3 a, Vec3 b) -> Vec3 {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

// Compiles to same assembly as:
// movss xmm0, [rdi]
// addss xmm0, [rsi]
// movss xmm1, [rdi+4]
// addss xmm1, [rsi+4]
// ...
```

### 5. Interoperability First

**Principle**: CU exists to bridge languages, not replace them.

CU is designed to work seamlessly with C, C++, and Assembly. It's a glue language.

**Example - Seamless Interop:**

```cu
// Call C standard library
import fn strlen(const char* str) -> u64;

// Export to C/C++
@nomangle
@export
fn cu_process_string(const char* str) -> u64 {
    return strlen(str) * 2;
}

// Use inline assembly
fn atomic_increment(i32* ptr) -> i32 {
    i32 result;
    asm(
        "lock xadd %0, %1"
        : "=r"(result), "+m"(*ptr)
        : "0"(1)
    );
    return result;
}
```

## Practical Examples

### Example 1: Simple Data Structure

**Goal**: Create a dynamic array (vector) in pure CU.

```cu
struct Vector {
    i32* data;
    u64 length;
    u64 capacity;
}

fn Vector_init(Vector* v) -> void {
    v->data = null;
    v->length = 0;
    v->capacity = 0;
}

fn Vector_push(Vector* v, i32 value) -> bool {
    // Need to grow?
    if (v->length >= v->capacity) {
        u64 new_capacity = v->capacity == 0 ? 8 : v->capacity * 2;
        i32* new_data = cast(i32*) malloc(sizeof(i32) * new_capacity);
        
        if (new_data == null) {
            return false;  // Allocation failed
        }
        
        // Copy old data
        for (u64 i = 0; i < v->length; i++) {
            new_data[i] = v->data[i];
        }
        
        // Free old data
        if (v->data != null) {
            free(v->data);
        }
        
        v->data = new_data;
        v->capacity = new_capacity;
    }
    
    v->data[v->length] = value;
    v->length++;
    return true;
}

fn Vector_get(Vector* v, u64 index) -> i32* {
    if (index >= v->length) {
        return null;
    }
    return &v->data[index];
}

fn Vector_free(Vector* v) -> void {
    if (v->data != null) {
        free(v->data);
    }
    v->data = null;
    v->length = 0;
    v->capacity = 0;
}

// Usage
fn example() -> void {
    Vector v;
    Vector_init(&v);
    
    Vector_push(&v, 10);
    Vector_push(&v, 20);
    Vector_push(&v, 30);
    
    i32* val = Vector_get(&v, 1);
    if (val != null) {
        // *val == 20
    }
    
    Vector_free(&v);
}
```

### Example 2: Error Handling Pattern

**Goal**: Demonstrate explicit error handling without exceptions.

```cu
enum ErrorCode {
    ERR_OK = 0,
    ERR_NULL_POINTER = 1,
    ERR_OUT_OF_BOUNDS = 2,
    ERR_OUT_OF_MEMORY = 3,
    ERR_INVALID_INPUT = 4
}

struct Result {
    ErrorCode error;
    i32 value;
}

fn safe_divide(i32 a, i32 b) -> Result {
    Result r;
    if (b == 0) {
        r.error = ERR_INVALID_INPUT;
        r.value = 0;
        return r;
    }
    
    r.error = ERR_OK;
    r.value = a / b;
    return r;
}

fn safe_array_access(i32* arr, u64 len, u64 index) -> Result {
    Result r;
    
    if (arr == null) {
        r.error = ERR_NULL_POINTER;
        r.value = 0;
        return r;
    }
    
    if (index >= len) {
        r.error = ERR_OUT_OF_BOUNDS;
        r.value = 0;
        return r;
    }
    
    r.error = ERR_OK;
    r.value = arr[index];
    return r;
}

// Usage
fn example() -> void {
    Result r = safe_divide(10, 2);
    if (r.error == ERR_OK) {
        // Use r.value
    } else {
        // Handle error
    }
    
    i32[5] arr = {1, 2, 3, 4, 5};
    Result r2 = safe_array_access(arr, 5, 10);
    if (r2.error == ERR_OUT_OF_BOUNDS) {
        // Handle out of bounds
    }
}
```

### Example 3: Memory Pool Allocator

**Goal**: Show how to implement a custom allocator.

```cu
struct MemoryPool {
    u8* memory;
    u64 block_size;
    u64 block_count;
    u64* free_blocks;  // Stack of free block indices
    u64 free_count;
}

fn MemoryPool_init(MemoryPool* pool, u64 block_size, u64 block_count) -> bool {
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->free_count = block_count;
    
    // Allocate memory
    pool->memory = cast(u8*) malloc(block_size * block_count);
    if (pool->memory == null) {
        return false;
    }
    
    // Allocate free list
    pool->free_blocks = cast(u64*) malloc(sizeof(u64) * block_count);
    if (pool->free_blocks == null) {
        free(pool->memory);
        return false;
    }
    
    // Initialize free list (all blocks are free)
    for (u64 i = 0; i < block_count; i++) {
        pool->free_blocks[i] = i;
    }
    
    return true;
}

fn MemoryPool_alloc(MemoryPool* pool) -> void* {
    if (pool->free_count == 0) {
        return null;  // Pool exhausted
    }
    
    // Pop from free list
    pool->free_count--;
    u64 block_index = pool->free_blocks[pool->free_count];
    
    // Return pointer to block
    return pool->memory + (block_index * pool->block_size);
}

fn MemoryPool_free(MemoryPool* pool, void* ptr) -> bool {
    if (ptr == null) {
        return false;
    }
    
    // Calculate block index
    u64 offset = cast(u8*) ptr - pool->memory;
    u64 block_index = offset / pool->block_size;
    
    // Validate
    if (block_index >= pool->block_count) {
        return false;  // Invalid pointer
    }
    
    // Push to free list
    pool->free_blocks[pool->free_count] = block_index;
    pool->free_count++;
    
    return true;
}

fn MemoryPool_destroy(MemoryPool* pool) -> void {
    if (pool->memory != null) {
        free(pool->memory);
    }
    if (pool->free_blocks != null) {
        free(pool->free_blocks);
    }
    pool->memory = null;
    pool->free_blocks = null;
}

// Usage
fn example() -> void {
    MemoryPool pool;
    if (!MemoryPool_init(&pool, 64, 100)) {
        return;  // Failed to initialize
    }
    
    // Allocate some blocks
    void* block1 = MemoryPool_alloc(&pool);
    void* block2 = MemoryPool_alloc(&pool);
    
    // Use blocks...
    
    // Free blocks
    MemoryPool_free(&pool, block1);
    MemoryPool_free(&pool, block2);
    
    MemoryPool_destroy(&pool);
}
```

### Example 4: Linked List

**Goal**: Classic data structure in CU style.

```cu
struct Node {
    i32 data;
    Node* next;
}

struct LinkedList {
    Node* head;
    u64 length;
}

fn LinkedList_init(LinkedList* list) -> void {
    list->head = null;
    list->length = 0;
}

fn LinkedList_push_front(LinkedList* list, i32 value) -> bool {
    Node* new_node = cast(Node*) malloc(sizeof(Node));
    if (new_node == null) {
        return false;
    }
    
    new_node->data = value;
    new_node->next = list->head;
    list->head = new_node;
    list->length++;
    
    return true;
}

fn LinkedList_pop_front(LinkedList* list, i32* out_value) -> bool {
    if (list->head == null) {
        return false;
    }
    
    Node* old_head = list->head;
    *out_value = old_head->data;
    list->head = old_head->next;
    free(old_head);
    list->length--;
    
    return true;
}

fn LinkedList_find(LinkedList* list, i32 value) -> Node* {
    Node* current = list->head;
    while (current != null) {
        if (current->data == value) {
            return current;
        }
        current = current->next;
    }
    return null;
}

fn LinkedList_free(LinkedList* list) -> void {
    Node* current = list->head;
    while (current != null) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    list->head = null;
    list->length = 0;
}

// Usage
fn example() -> void {
    LinkedList list;
    LinkedList_init(&list);
    
    LinkedList_push_front(&list, 10);
    LinkedList_push_front(&list, 20);
    LinkedList_push_front(&list, 30);
    
    Node* found = LinkedList_find(&list, 20);
    if (found != null) {
        // Found the node
    }
    
    i32 value;
    while (LinkedList_pop_front(&list, &value)) {
        // Process value
    }
    
    LinkedList_free(&list);
}
```

### Example 5: String Handling

**Goal**: Safe string operations without C's pitfalls.

```cu
struct String {
    char* data;
    u64 length;
    u64 capacity;
}

fn String_init(String* s) -> void {
    s->data = null;
    s->length = 0;
    s->capacity = 0;
}

fn String_from_cstr(const char* cstr) -> String {
    String s;
    String_init(&s);
    
    if (cstr == null) {
        return s;
    }
    
    // Calculate length
    u64 len = 0;
    while (cstr[len] != '\0') {
        len++;
    }
    
    // Allocate
    s.data = cast(char*) malloc(len + 1);
    if (s.data == null) {
        return s;
    }
    
    // Copy
    for (u64 i = 0; i < len; i++) {
        s.data[i] = cstr[i];
    }
    s.data[len] = '\0';
    
    s.length = len;
    s.capacity = len + 1;
    
    return s;
}

fn String_append(String* s, const char* cstr) -> bool {
    if (cstr == null) {
        return true;
    }
    
    // Calculate additional length needed
    u64 add_len = 0;
    while (cstr[add_len] != '\0') {
        add_len++;
    }
    
    u64 new_length = s->length + add_len;
    
    // Grow if needed
    if (new_length + 1 > s->capacity) {
        u64 new_capacity = new_length + 1;
        char* new_data = cast(char*) malloc(new_capacity);
        if (new_data == null) {
            return false;
        }
        
        // Copy existing data
        for (u64 i = 0; i < s->length; i++) {
            new_data[i] = s->data[i];
        }
        
        if (s->data != null) {
            free(s->data);
        }
        
        s->data = new_data;
        s->capacity = new_capacity;
    }
    
    // Append new data
    for (u64 i = 0; i < add_len; i++) {
        s->data[s->length + i] = cstr[i];
    }
    s->data[new_length] = '\0';
    s->length = new_length;
    
    return true;
}

fn String_free(String* s) -> void {
    if (s->data != null) {
        free(s->data);
    }
    s->data = null;
    s->length = 0;
    s->capacity = 0;
}

// Usage
fn example() -> void {
    String s = String_from_cstr("Hello");
    String_append(&s, ", ");
    String_append(&s, "World!");
    
    // s.data == "Hello, World!"
    // s.length == 13
    
    String_free(&s);
}
```

### Example 6: Callback Pattern

**Goal**: Demonstrate function pointers for callbacks.

```cu
// Callback type
typedef fn(i32) -> void Callback;

struct EventSystem {
    Callback* callbacks;
    u64 callback_count;
    u64 callback_capacity;
}

fn EventSystem_init(EventSystem* es) -> void {
    es->callbacks = null;
    es->callback_count = 0;
    es->callback_capacity = 0;
}

fn EventSystem_register(EventSystem* es, Callback cb) -> bool {
    if (es->callback_count >= es->callback_capacity) {
        u64 new_capacity = es->callback_capacity == 0 ? 4 : es->callback_capacity * 2;
        Callback* new_callbacks = cast(Callback*) malloc(sizeof(Callback) * new_capacity);
        
        if (new_callbacks == null) {
            return false;
        }
        
        for (u64 i = 0; i < es->callback_count; i++) {
            new_callbacks[i] = es->callbacks[i];
        }
        
        if (es->callbacks != null) {
            free(es->callbacks);
        }
        
        es->callbacks = new_callbacks;
        es->callback_capacity = new_capacity;
    }
    
    es->callbacks[es->callback_count] = cb;
    es->callback_count++;
    return true;
}

fn EventSystem_trigger(EventSystem* es, i32 event_data) -> void {
    for (u64 i = 0; i < es->callback_count; i++) {
        es->callbacks[i](event_data);
    }
}

fn EventSystem_free(EventSystem* es) -> void {
    if (es->callbacks != null) {
        free(es->callbacks);
    }
    es->callbacks = null;
    es->callback_count = 0;
    es->callback_capacity = 0;
}

// Example callbacks
fn on_event_a(i32 data) -> void {
    // Handle event A
}

fn on_event_b(i32 data) -> void {
    // Handle event B
}

// Usage
fn example() -> void {
    EventSystem es;
    EventSystem_init(&es);
    
    EventSystem_register(&es, on_event_a);
    EventSystem_register(&es, on_event_b);
    
    // Trigger event
    EventSystem_trigger(&es, 42);
    
    EventSystem_free(&es);
}
```

## Design Patterns in CU

### Pattern 1: Constructor/Destructor

```cu
struct Object {
    i32* data;
}

// Constructor
fn Object_init(Object* obj, u64 size) -> bool {
    obj->data = cast(i32*) malloc(sizeof(i32) * size);
    return obj->data != null;
}

// Destructor
fn Object_free(Object* obj) -> void {
    if (obj->data != null) {
        free(obj->data);
        obj->data = null;
    }
}
```

### Pattern 2: Result Type

```cu
struct Result_i32 {
    bool ok;
    i32 value;
    ErrorCode error;
}

fn try_parse_int(const char* str) -> Result_i32 {
    Result_i32 r;
    if (str == null) {
        r.ok = false;
        r.error = ERR_NULL_POINTER;
        return r;
    }
    
    // Parse logic...
    r.ok = true;
    r.value = 42;  // Parsed value
    return r;
}
```

### Pattern 3: Iterator

```cu
struct Iterator {
    void* data;
    fn(void*) -> bool has_next;
    fn(void*) -> i32 next;
}

fn Iterator_foreach(Iterator* it, fn(i32) -> void callback) -> void {
    while (it->has_next(it->data)) {
        i32 value = it->next(it->data);
        callback(value);
    }
}
```

## Philosophy Summary

CU is designed to be:

1. **Simple** - Minimal features, maximum clarity
2. **Portable** - Same behavior everywhere
3. **Explicit** - No hidden costs or surprises
4. **Efficient** - Zero-cost abstractions
5. **Interoperable** - Seamless with C/C++/Assembly

It's not trying to be the best language for everything. It's trying to be the best **bridge** between low-level languages and higher-level code.
