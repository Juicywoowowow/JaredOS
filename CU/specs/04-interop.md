# CU Interoperability Specification

## Philosophy

CU is designed as a **gateway language** - it exists to bridge C, C++, and Assembly seamlessly. Interoperability is not a feature, it's the core purpose.

## C Interoperability

### Calling C from CU

```cu
// Import C functions
import fn malloc(u64 size) -> void*;
import fn free(void* ptr) -> void;
import fn printf(const char* fmt, ...) -> i32;

fn example() -> void {
    void* ptr = malloc(100);
    printf("Allocated at: %p\n", ptr);
    free(ptr);
}
```

### Calling CU from C

```c
// CU code (example.cu)
@nomangle
@export
fn cu_add(i32 a, i32 b) -> i32 {
    return a + b;
}
```

```c
// C code (main.c)
#include <stdint.h>

// Declare CU function
extern int32_t cu_add(int32_t a, int32_t b);

int main() {
    int32_t result = cu_add(5, 3);
    return 0;
}
```

### Type Mapping: CU ↔ C

| CU Type | C Type | Notes |
|---------|--------|-------|
| `i8` | `int8_t` | Requires `<stdint.h>` |
| `i16` | `int16_t` | |
| `i32` | `int32_t` | |
| `i64` | `int64_t` | |
| `u8` | `uint8_t` | |
| `u16` | `uint16_t` | |
| `u32` | `uint32_t` | |
| `u64` | `uint64_t` | |
| `f32` | `float` | |
| `f64` | `double` | |
| `bool` | `_Bool` or `bool` | Requires `<stdbool.h>` in C |
| `void` | `void` | |
| `void*` | `void*` | |
| `struct` | `struct` | Same layout |
| `union` | `union` | Same layout |
| `enum` | `enum` | Compatible with `int` |

### Struct Compatibility

```cu
// CU code
@nomangle
struct Point {
    i32 x;
    i32 y;
}

@export
fn create_point(i32 x, i32 y) -> Point {
    Point p = {x, y};
    return p;
}
```

```c
// C code
#include <stdint.h>

struct Point {
    int32_t x;
    int32_t y;
};

extern struct Point create_point(int32_t x, int32_t y);

int main() {
    struct Point p = create_point(10, 20);
    return 0;
}
```

### Variadic Functions

```cu
// Importing C variadic functions
import fn printf(const char* fmt, ...) -> i32;

fn example() -> void {
    printf("Number: %d, Float: %f\n", 42, 3.14);
}

// CU does NOT support defining variadic functions
// Use C wrapper if needed
```

## C++ Interoperability

### Calling C++ from CU

```cpp
// C++ code (wrapper.cpp)
extern "C" {
    void cpp_function(int x) {
        // C++ implementation
        std::cout << x << std::endl;
    }
}
```

```cu
// CU code
import fn cpp_function(i32 x) -> void;

fn example() -> void {
    cpp_function(42);
}
```

### Calling CU from C++

```cu
// CU code (math.cu)
@nomangle
@export
fn cu_multiply(i32 a, i32 b) -> i32 {
    return a * b;
}
```

```cpp
// C++ code (main.cpp)
extern "C" {
    int32_t cu_multiply(int32_t a, int32_t b);
}

int main() {
    int32_t result = cu_multiply(5, 3);
    return 0;
}
```

### C++ Class Interop (Opaque Pointers)

```cpp
// C++ code (widget.cpp)
class Widget {
public:
    void doSomething();
};

extern "C" {
    void* widget_create() {
        return new Widget();
    }
    
    void widget_destroy(void* widget) {
        delete static_cast<Widget*>(widget);
    }
    
    void widget_do_something(void* widget) {
        static_cast<Widget*>(widget)->doSomething();
    }
}
```

```cu
// CU code
struct Widget;  // Opaque type

import fn widget_create() -> Widget*;
import fn widget_destroy(Widget* w) -> void;
import fn widget_do_something(Widget* w) -> void;

fn example() -> void {
    Widget* w = widget_create();
    widget_do_something(w);
    widget_destroy(w);
}
```

### Type Mapping: CU ↔ C++

Same as C, plus:

| CU Type | C++ Type | Notes |
|---------|----------|-------|
| `bool` | `bool` | Direct mapping |
| Opaque struct | Class pointer | Via `void*` or opaque struct |

## Assembly Interoperability

### Inline Assembly

```cu
fn get_cpu_id() -> u32 {
    u32 result;
    asm(
        "mov eax, 1\n"
        "cpuid\n"
        "mov %0, eax\n"
        : "=r"(result)  // Output
        :               // Input
        : "eax", "ebx", "ecx", "edx"  // Clobbers
    );
    return result;
}
```

### Inline Assembly Syntax

```cu
asm(
    "assembly code"
    : output operands
    : input operands
    : clobbered registers
);
```

#### Output Operands

```cu
u32 result;
asm("mov %0, eax" : "=r"(result));
// =r: write-only register
// +r: read-write register
// =m: write-only memory
```

#### Input Operands

```cu
u32 input = 42;
asm("mov eax, %0" : : "r"(input));
// r: register
// m: memory
// i: immediate constant
```

#### Clobbers

```cu
asm(
    "mov eax, 1\n"
    "cpuid"
    : // outputs
    : // inputs
    : "eax", "ebx", "ecx", "edx", "memory"
);
```

### Calling Assembly Functions

```asm
; assembly code (add.asm)
global asm_add

section .text
asm_add:
    mov eax, edi    ; First argument (x86-64 calling convention)
    add eax, esi    ; Second argument
    ret
```

```cu
// CU code
import fn asm_add(i32 a, i32 b) -> i32;

fn example() -> void {
    i32 result = asm_add(5, 3);
}
```

### Calling CU from Assembly

```cu
// CU code (math.cu)
@nomangle
@export
fn cu_multiply(i32 a, i32 b) -> i32 {
    return a * b;
}
```

```asm
; Assembly code
extern cu_multiply

section .text
global main
main:
    mov edi, 5      ; First argument
    mov esi, 3      ; Second argument
    call cu_multiply
    ret
```

### Register Conventions

#### x86-64 System V ABI (Linux, macOS)

| Register | Purpose | Preserved? |
|----------|---------|------------|
| `rax` | Return value | No |
| `rdi` | 1st argument | No |
| `rsi` | 2nd argument | No |
| `rdx` | 3rd argument | No |
| `rcx` | 4th argument | No |
| `r8` | 5th argument | No |
| `r9` | 6th argument | No |
| `rbx` | General purpose | Yes |
| `rbp` | Frame pointer | Yes |
| `rsp` | Stack pointer | Yes |
| `r12-r15` | General purpose | Yes |

#### x86-64 Microsoft ABI (Windows)

| Register | Purpose | Preserved? |
|----------|---------|------------|
| `rax` | Return value | No |
| `rcx` | 1st argument | No |
| `rdx` | 2nd argument | No |
| `r8` | 3rd argument | No |
| `r9` | 4th argument | No |
| `rbx` | General purpose | Yes |
| `rbp` | Frame pointer | Yes |
| `rsp` | Stack pointer | Yes |
| `rsi`, `rdi` | General purpose | Yes |
| `r12-r15` | General purpose | Yes |

### SIMD Intrinsics

```cu
// Import SIMD functions
import fn _mm_add_ps(f32[4] a, f32[4] b) -> f32[4];

fn simd_example() -> void {
    f32[4] a = {1.0, 2.0, 3.0, 4.0};
    f32[4] b = {5.0, 6.0, 7.0, 8.0};
    f32[4] result = _mm_add_ps(a, b);
}
```

## Calling Conventions

### Specifying Calling Convention

```cu
@cdecl
fn c_convention(i32 x) -> i32 {
    return x * 2;
}

@stdcall
fn std_convention(i32 x) -> i32 {
    return x * 2;
}

@fastcall
fn fast_convention(i32 x) -> i32 {
    return x * 2;
}
```

### Default Calling Convention

- **Linux/macOS**: System V ABI (cdecl-like)
- **Windows**: Microsoft x64 calling convention
- Can be overridden with attributes

## Foreign Function Interface (FFI)

### Loading Dynamic Libraries

```cu
// Platform-specific
#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    import fn dlopen(const char* filename, i32 flags) -> void*;
    import fn dlsym(void* handle, const char* symbol) -> void*;
    import fn dlclose(void* handle) -> i32;
#elif defined(PLATFORM_WINDOWS)
    import fn LoadLibraryA(const char* filename) -> void*;
    import fn GetProcAddress(void* handle, const char* name) -> void*;
    import fn FreeLibrary(void* handle) -> i32;
#endif

fn example() -> void {
    void* lib = dlopen("libexample.so", 2);  // RTLD_NOW
    if (lib != null) {
        fn(i32) -> i32 func = cast(fn(i32) -> i32) dlsym(lib, "function_name");
        if (func != null) {
            i32 result = func(42);
        }
        dlclose(lib);
    }
}
```

### Function Pointer Casting

```cu
void* raw_ptr = dlsym(lib, "add");
fn(i32, i32) -> i32 typed_func = cast(fn(i32, i32) -> i32) raw_ptr;
```

## Name Mangling

### Preventing Name Mangling

```cu
@nomangle
fn exported_function() -> void {
    // Symbol name: "exported_function"
}

// Without @nomangle, symbol might be:
// "_cu_exported_function" or similar
```

### Export Visibility

```cu
@export
@nomangle
fn public_api() -> void {
    // Visible to external code
}

// Internal function (not exported)
fn internal_helper() -> void {
    // Only visible within module
}
```

## Platform-Specific Code

### Conditional Compilation

```cu
#if defined(PLATFORM_LINUX)
    fn platform_init() -> void {
        // Linux-specific
    }
#elif defined(PLATFORM_WINDOWS)
    fn platform_init() -> void {
        // Windows-specific
    }
#elif defined(PLATFORM_MACOS)
    fn platform_init() -> void {
        // macOS-specific
    }
#else
    #error "Unsupported platform"
#endif
```

### Architecture-Specific Code

```cu
#if defined(ARCH_X86_64)
    fn get_timestamp() -> u64 {
        u64 result;
        asm("rdtsc\n"
            "shl rdx, 32\n"
            "or rax, rdx\n"
            "mov %0, rax"
            : "=r"(result)
            :
            : "rax", "rdx"
        );
        return result;
    }
#elif defined(ARCH_ARM64)
    fn get_timestamp() -> u64 {
        u64 result;
        asm("mrs %0, cntvct_el0" : "=r"(result));
        return result;
    }
#endif
```

## Best Practices

### 1. Use Opaque Types for Complex Objects

```cu
struct OpaqueHandle;  // Forward declaration

import fn create_handle() -> OpaqueHandle*;
import fn destroy_handle(OpaqueHandle* h) -> void;
```

### 2. Always Use @nomangle for Exported Functions

```cu
@nomangle
@export
fn api_function() -> void {
    // ...
}
```

### 3. Match Calling Conventions

```cu
// When calling Windows API
@stdcall
import fn MessageBoxA(void* hwnd, const char* text, const char* caption, u32 type) -> i32;
```

### 4. Use Explicit Types

```cu
// Good: Explicit sizes
fn process(i32* data, u64 count) -> void;

// Bad: Platform-dependent sizes
// fn process(int* data, size_t count) -> void;
```

### 5. Document ABI Requirements

```cu
// This function expects System V ABI (Linux/macOS)
@cdecl
@nomangle
@export
fn linux_api(i32 x, i32 y) -> i32 {
    return x + y;
}
```
