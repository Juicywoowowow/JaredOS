# CU Memory Model

## Memory Layout

### Address Space

CU uses a flat memory model with a single address space:

```
High Address
+------------------+
|      Stack       |  <- Grows downward
|        ↓         |
+------------------+
|                  |
|   (Free Space)   |
|                  |
+------------------+
|        ↑         |
|      Heap        |  <- Grows upward
+------------------+
|   Static Data    |
+------------------+
|   Read-Only      |
|      Data        |
+------------------+
|   Code Segment   |
+------------------+
Low Address
```

## Storage Classes

### Automatic Storage

```cu
fn example() -> void {
    i32 x;          // Automatic storage
    i32 arr[100];   // Stack-allocated array
    
    // Lifetime: function scope
    // Destroyed when function returns
}
```

### Static Storage

```cu
static i32 counter = 0;  // File scope, static storage

fn increment() -> void {
    static i32 local_static = 0;  // Function scope, static storage
    local_static++;
}
```

### Dynamic Storage

```cu
// Heap allocation (requires import)
import fn malloc(u64 size) -> void*;
import fn free(void* ptr) -> void;

fn example() -> void {
    i32* ptr = cast(i32*) malloc(sizeof(i32) * 100);
    // Use ptr...
    free(ptr);
}
```

### Thread-Local Storage

```cu
@thread_local
static i32 thread_counter = 0;
```

## Memory Alignment

### Natural Alignment

```cu
struct Aligned {
    i8 a;    // offset 0, align 1
    // 3 bytes padding
    i32 b;   // offset 4, align 4
    i16 c;   // offset 8, align 2
    // 2 bytes padding
}  // total size: 12 bytes
```

### Custom Alignment

```cu
@align(16)
struct SIMD_Data {
    f32[4] values;  // Aligned to 16-byte boundary
}

@align(64)  // Cache line alignment
struct CacheAligned {
    i32 data;
}
```

### Packed Structures

```cu
@packed
struct Packed {
    i8 a;    // offset 0
    i32 b;   // offset 1 (no padding!)
    i16 c;   // offset 5
}  // total size: 7 bytes

// Warning: Unaligned access may be slower or cause errors on some platforms
```

## Memory Operations

### Basic Operations

```cu
// Address-of operator
i32 x = 42;
i32* ptr = &x;

// Dereference operator
i32 value = *ptr;

// Pointer arithmetic
i32[10] arr;
i32* p = &arr[0];
p++;           // Points to arr[1]
p += 5;        // Points to arr[6]

// Array indexing (syntactic sugar for pointer arithmetic)
arr[3] = 100;  // Equivalent to: *(arr + 3) = 100
```

### Memory Copy

```cu
// Manual memory copy
fn memcpy(void* dest, void* src, u64 size) -> void {
    u8* d = cast(u8*) dest;
    u8* s = cast(u8*) src;
    for (u64 i = 0; i < size; i++) {
        d[i] = s[i];
    }
}
```

### Memory Set

```cu
fn memset(void* ptr, u8 value, u64 size) -> void {
    u8* p = cast(u8*) ptr;
    for (u64 i = 0; i < size; i++) {
        p[i] = value;
    }
}
```

### Memory Compare

```cu
fn memcmp(void* a, void* b, u64 size) -> i32 {
    u8* pa = cast(u8*) a;
    u8* pb = cast(u8*) b;
    for (u64 i = 0; i < size; i++) {
        if (pa[i] != pb[i]) {
            return pa[i] - pb[i];
        }
    }
    return 0;
}
```

## Memory Safety

### Undefined Behavior

CU does **not** prevent undefined behavior. The following are programmer responsibilities:

1. **Null pointer dereference**
   ```cu
   i32* ptr = null;
   *ptr = 42;  // UNDEFINED BEHAVIOR
   ```

2. **Out-of-bounds access**
   ```cu
   i32[10] arr;
   arr[100] = 42;  // UNDEFINED BEHAVIOR
   ```

3. **Use after free**
   ```cu
   i32* ptr = cast(i32*) malloc(sizeof(i32));
   free(ptr);
   *ptr = 42;  // UNDEFINED BEHAVIOR
   ```

4. **Uninitialized memory**
   ```cu
   i32 x;
   i32 y = x;  // UNDEFINED BEHAVIOR (x not initialized)
   ```

5. **Type punning (except through unions)**
   ```cu
   i32 x = 42;
   f32 f = *cast(f32*) &x;  // UNDEFINED BEHAVIOR
   
   // Correct way:
   union Convert {
       i32 as_int;
       f32 as_float;
   }
   Convert c;
   c.as_int = 42;
   f32 f = c.as_float;  // OK
   ```

### Safe Patterns

```cu
// Always check for null
i32* ptr = cast(i32*) malloc(sizeof(i32));
if (ptr != null) {
    *ptr = 42;
    free(ptr);
}

// Bounds checking
fn safe_access(i32* arr, u64 len, u64 index) -> i32* {
    if (index < len) {
        return &arr[index];
    }
    return null;
}

// Initialize variables
i32 x = 0;  // Always initialize
```

## Memory Ordering

### Volatile Access

```cu
// Volatile prevents optimization
volatile u32* hardware_register = cast(volatile u32*) 0x40000000;

u32 value = *hardware_register;  // Always reads from memory
*hardware_register = 42;         // Always writes to memory
```

### Memory Barriers

```cu
// Inline assembly for memory barriers
fn memory_barrier() -> void {
    asm("mfence");  // x86-64 memory fence
}

fn read_barrier() -> void {
    asm("lfence");  // x86-64 load fence
}

fn write_barrier() -> void {
    asm("sfence");  // x86-64 store fence
}
```

## Stack Management

### Stack Allocation

```cu
fn example() -> void {
    i32[1000] large_array;  // Stack allocation
    // Warning: Large stack allocations may overflow
}
```

### Variable-Length Arrays (VLA)

**Not supported** in CU. Use heap allocation instead:

```cu
// Wrong:
// i32[n] arr;  // ERROR: VLA not supported

// Correct:
i32* arr = cast(i32*) malloc(sizeof(i32) * n);
// ... use arr ...
free(arr);
```

### Stack Overflow Protection

CU does not provide automatic stack overflow protection. Programmers must:

1. Avoid large stack allocations
2. Use heap for large data structures
3. Be careful with deep recursion

## Heap Management

### Allocation

```cu
import fn malloc(u64 size) -> void*;
import fn calloc(u64 count, u64 size) -> void*;  // Zero-initialized
import fn realloc(void* ptr, u64 new_size) -> void*;
import fn free(void* ptr) -> void;

// Example
i32* arr = cast(i32*) malloc(sizeof(i32) * 100);
if (arr != null) {
    // Use arr...
    free(arr);
}
```

### Custom Allocators

```cu
struct Allocator {
    fn(u64) -> void* alloc;
    fn(void*) -> void dealloc;
}

fn arena_alloc(u64 size) -> void* {
    // Arena allocation logic
}

fn arena_dealloc(void* ptr) -> void {
    // No-op for arena allocator
}

Allocator arena = {arena_alloc, arena_dealloc};
```

## Memory-Mapped I/O

### Direct Memory Access

```cu
// Memory-mapped hardware register
volatile u32* gpio_register = cast(volatile u32*) 0x40020000;

// Read
u32 value = *gpio_register;

// Write
*gpio_register = 0xFF;

// Read-modify-write
*gpio_register |= (1 << 5);
```

### DMA Buffers

```cu
@align(64)  // Cache line aligned
struct DMA_Buffer {
    u8[4096] data;
}

DMA_Buffer* buffer = cast(DMA_Buffer*) malloc(sizeof(DMA_Buffer));
```

## Endianness

### Byte Order Conversion

```cu
// Big-endian to host
fn be32_to_host(u32 value) -> u32 {
    #if defined(LITTLE_ENDIAN)
        return ((value & 0xFF000000) >> 24) |
               ((value & 0x00FF0000) >> 8)  |
               ((value & 0x0000FF00) << 8)  |
               ((value & 0x000000FF) << 24);
    #else
        return value;
    #endif
}

// Little-endian to host
fn le32_to_host(u32 value) -> u32 {
    #if defined(BIG_ENDIAN)
        return ((value & 0xFF000000) >> 24) |
               ((value & 0x00FF0000) >> 8)  |
               ((value & 0x0000FF00) << 8)  |
               ((value & 0x000000FF) << 24);
    #else
        return value;
    #endif
}
```

## Memory Pools

### Fixed-Size Pool

```cu
struct Pool {
    void* memory;
    u64 block_size;
    u64 block_count;
    u64 free_count;
    void** free_list;
}

fn pool_init(Pool* pool, u64 block_size, u64 block_count) -> bool {
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->free_count = block_count;
    
    pool->memory = malloc(block_size * block_count);
    if (pool->memory == null) return false;
    
    pool->free_list = cast(void**) malloc(sizeof(void*) * block_count);
    if (pool->free_list == null) {
        free(pool->memory);
        return false;
    }
    
    // Initialize free list
    for (u64 i = 0; i < block_count; i++) {
        pool->free_list[i] = cast(void*)(cast(u8*)pool->memory + i * block_size);
    }
    
    return true;
}

fn pool_alloc(Pool* pool) -> void* {
    if (pool->free_count == 0) return null;
    pool->free_count--;
    return pool->free_list[pool->free_count];
}

fn pool_free(Pool* pool, void* ptr) -> void {
    pool->free_list[pool->free_count] = ptr;
    pool->free_count++;
}
```

## Zero-Cost Abstractions

CU guarantees that abstractions have zero runtime cost:

```cu
// This struct wrapper...
struct SafeInt {
    i32 value;
}

fn SafeInt_add(SafeInt a, SafeInt b) -> SafeInt {
    SafeInt result;
    result.value = a.value + b.value;
    return result;
}

// ...compiles to the same code as:
i32 a = 10;
i32 b = 20;
i32 result = a + b;
```
