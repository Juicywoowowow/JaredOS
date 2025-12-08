# CU Type System

## Primitive Types

### Integer Types

| Type | Size | Range | Description |
|------|------|-------|-------------|
| `i8` | 1 byte | -128 to 127 | Signed 8-bit integer |
| `i16` | 2 bytes | -32,768 to 32,767 | Signed 16-bit integer |
| `i32` | 4 bytes | -2,147,483,648 to 2,147,483,647 | Signed 32-bit integer |
| `i64` | 8 bytes | -2^63 to 2^63-1 | Signed 64-bit integer |
| `u8` | 1 byte | 0 to 255 | Unsigned 8-bit integer |
| `u16` | 2 bytes | 0 to 65,535 | Unsigned 16-bit integer |
| `u32` | 4 bytes | 0 to 4,294,967,295 | Unsigned 32-bit integer |
| `u64` | 8 bytes | 0 to 2^64-1 | Unsigned 64-bit integer |

### Floating-Point Types

| Type | Size | Precision | Description |
|------|------|-----------|-------------|
| `f32` | 4 bytes | ~7 decimal digits | IEEE 754 single precision |
| `f64` | 8 bytes | ~15 decimal digits | IEEE 754 double precision |

### Boolean Type

| Type | Size | Values | Description |
|------|------|--------|-------------|
| `bool` | 1 byte | `true`, `false` | Boolean value |

**Note**: `bool` is 1 byte for ABI compatibility with C/C++.

### Void Type

```cu
void  // Represents absence of type
```

- Cannot create variables of type `void`
- Used for functions with no return value
- `void*` is a generic pointer type

## Pointer Types

### Raw Pointers

```cu
i32* ptr;           // Pointer to i32
void* generic;      // Generic pointer
i32** ptr_to_ptr;   // Pointer to pointer
```

### Null Pointers

```cu
i32* ptr = null;    // Null pointer
```

### Pointer Arithmetic

```cu
i32* ptr = &array[0];
ptr++;              // Move to next element
ptr += 5;           // Move 5 elements forward
i64 diff = ptr2 - ptr1;  // Difference in elements
```

## Array Types

### Fixed-Size Arrays

```cu
i32[10] array;      // Array of 10 integers
f32[3][3] matrix;   // 3x3 matrix

// Initialization
i32[5] nums = {1, 2, 3, 4, 5};
i32[3] partial = {1, 2};  // Rest initialized to 0
```

### Array Decay

Fixed arrays decay to pointers when passed to functions:

```cu
fn process(i32* arr, u64 len) -> void {
    // ...
}

i32[10] data;
process(data, 10);  // Array decays to pointer
```

### Slices (Fat Pointers)

```cu
struct Slice_i32 {
    i32* ptr;
    u64 len;
}

// Syntax sugar
i32[] slice;  // Equivalent to Slice_i32
```

## Struct Types

### Basic Structures

```cu
struct Point {
    i32 x;
    i32 y;
}

// Usage
Point p = {10, 20};
p.x = 30;
```

### Nested Structures

```cu
struct Rectangle {
    Point top_left;
    Point bottom_right;
}

Rectangle r = {{0, 0}, {100, 100}};
```

### Anonymous Structures

```cu
struct {
    i32 x;
    i32 y;
} anonymous_point;
```

### Structure Alignment

```cu
// Default alignment
struct Default {
    u8 a;   // offset 0
    u32 b;  // offset 4 (aligned)
    u16 c;  // offset 8
}  // size: 12 bytes (with padding)

// Packed structure
@packed
struct Packed {
    u8 a;   // offset 0
    u32 b;  // offset 1 (no alignment)
    u16 c;  // offset 5
}  // size: 7 bytes

// Custom alignment
@align(16)
struct Aligned {
    f32[4] data;
}  // aligned to 16-byte boundary
```

## Union Types

### Basic Unions

```cu
union Value {
    i32 as_int;
    f32 as_float;
    u8[4] as_bytes;
}

Value v;
v.as_int = 42;
f32 f = v.as_float;  // Type punning
```

### Tagged Unions (Manual)

```cu
enum ValueType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_PTR
}

struct TaggedValue {
    ValueType type;
    union {
        i32 as_int;
        f32 as_float;
        void* as_ptr;
    } data;
}
```

## Enum Types

### Basic Enums

```cu
enum Color {
    RED,      // 0
    GREEN,    // 1
    BLUE      // 2
}

Color c = RED;
```

### Explicit Values

```cu
enum Status {
    OK = 0,
    ERROR = -1,
    PENDING = 1,
    TIMEOUT = 2
}
```

### Enum as Flags

```cu
enum Flags {
    FLAG_A = 1 << 0,  // 0x01
    FLAG_B = 1 << 1,  // 0x02
    FLAG_C = 1 << 2,  // 0x04
    FLAG_D = 1 << 3   // 0x08
}

u32 flags = FLAG_A | FLAG_C;
```

## Function Types

### Function Pointers

```cu
// Function pointer type
fn(i32, i32) -> i32 binary_op;

// Assignment
i32 add(i32 a, i32 b) { return a + b; }
binary_op = add;

// Call through pointer
i32 result = binary_op(5, 3);
```

### Callback Example

```cu
fn apply(i32 x, fn(i32) -> i32 func) -> i32 {
    return func(x);
}

fn square(i32 n) -> i32 {
    return n * n;
}

i32 result = apply(5, square);  // 25
```

## Type Aliases

```cu
// Type alias (compile-time only)
typedef i32 Integer;
typedef fn(i32, i32) -> i32 BinaryOp;

Integer x = 42;
BinaryOp op = add;
```

## Type Casting

### Explicit Casting

```cu
// Numeric casts
i32 x = 42;
f32 f = cast(f32) x;

// Pointer casts
void* generic = malloc(100);
i32* typed = cast(i32*) generic;

// Truncation
i64 big = 1000;
i32 small = cast(i32) big;
```

### Implicit Conversions

CU has **very limited** implicit conversions:

1. **Integer promotion**: Smaller integers promote to larger in expressions
2. **Array to pointer**: Arrays decay to pointers
3. **Function to pointer**: Function names become function pointers

**No implicit conversions**:
- Between signed and unsigned
- Between integer and float
- Between pointer types
- Between bool and integer

## Type Sizes and Alignment

### Size Guarantees

```cu
sizeof(i8)  == 1
sizeof(i16) == 2
sizeof(i32) == 4
sizeof(i64) == 8
sizeof(f32) == 4
sizeof(f64) == 8
sizeof(bool) == 1
sizeof(ptr) == platform dependent (4 or 8)
```

### Alignment Rules

```cu
// Natural alignment
alignof(i8)  == 1
alignof(i16) == 2
alignof(i32) == 4
alignof(i64) == 8
alignof(f32) == 4
alignof(f64) == 8

// Struct alignment = max(member alignments)
struct Example {
    i8 a;   // align 1
    i64 b;  // align 8
}
alignof(Example) == 8
```

## Type Compatibility

### Compatibility Rules

1. **Exact match**: Types must match exactly (no implicit conversions)
2. **Pointer compatibility**: `void*` can be explicitly cast to/from any pointer
3. **Struct compatibility**: Structs are compatible if they have identical layout
4. **Enum compatibility**: Enums are compatible with their underlying integer type

### C/C++ Compatibility

CU types map directly to C types:

| CU Type | C Type | C++ Type |
|---------|--------|----------|
| `i8` | `int8_t` | `int8_t` |
| `i16` | `int16_t` | `int16_t` |
| `i32` | `int32_t` | `int32_t` |
| `i64` | `int64_t` | `int64_t` |
| `u8` | `uint8_t` | `uint8_t` |
| `u16` | `uint16_t` | `uint16_t` |
| `u32` | `uint32_t` | `uint32_t` |
| `u64` | `uint64_t` | `uint64_t` |
| `f32` | `float` | `float` |
| `f64` | `double` | `double` |
| `bool` | `_Bool` / `bool` | `bool` |
| `void` | `void` | `void` |
| `ptr` | `void*` | `void*` |

## Type Traits

### Compile-Time Type Information

```cu
// Size of type
const u64 size = sizeof(i32);

// Alignment of type
const u64 align = alignof(i64);

// Type of expression
typeof(x + y) result;
```

## Opaque Types

For hiding implementation details:

```cu
// In header (.cuh)
struct Handle;  // Opaque type

fn create_handle() -> Handle*;
fn destroy_handle(Handle* h) -> void;

// In implementation (.cu)
struct Handle {
    i32 internal_data;
    // ...
}
```
