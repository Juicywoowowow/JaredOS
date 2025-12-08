# CU Syntax Specification

## File Extension

- `.cup` - CU source files (CU Program)
- `.cuph` - CU header files (declarations only)

## Comments

```cu
// Single-line comment

/* Multi-line
   comment */
```

## Keywords

### Type Keywords
```
void bool
i8 i16 i32 i64
u8 u16 u32 u64
f32 f64
ptr
```

### Control Flow
```
if else
while for
break continue
return
goto
```

### Declaration Keywords
```
fn        // Function
struct    // Structure
union     // Union
enum      // Enumeration
const     // Compile-time constant
static    // Static storage
extern    // External linkage
inline    // Inline hint
```

### Special Keywords
```
sizeof
typeof
cast
asm      // Inline assembly
export   // Export symbol for FFI
import   // Import from C/C++/Assembly
```

## Identifiers

- Start with letter or underscore: `[a-zA-Z_]`
- Followed by letters, digits, or underscores: `[a-zA-Z0-9_]*`
- Case-sensitive
- No length limit (implementation may impose reasonable limits)

### Reserved Prefixes
- `__cu_` - Reserved for compiler internals
- `_CU_` - Reserved for implementation macros

## Literals

### Integer Literals
```cu
42          // Decimal
0x2A        // Hexadecimal
0b101010    // Binary
0o52        // Octal
100_000     // Underscores for readability
```

### Floating-Point Literals
```cu
3.14
1.0e-5
0.5f        // f32 suffix
2.0         // f64 default
```

### Boolean Literals
```cu
true
false
```

### Character Literals
```cu
'a'
'\n'        // Newline
'\t'        // Tab
'\0'        // Null
'\x41'      // Hex escape
```

### String Literals
```cu
"Hello, World!"
"Line 1\nLine 2"
"Embedded\0null"
```

### Null Pointer
```cu
null        // Null pointer constant
```

## Operators

### Arithmetic
```cu
+  -  *  /  %       // Binary
+  -                // Unary
++  --              // Increment/Decrement (prefix and postfix)
```

### Bitwise
```cu
&  |  ^  ~          // AND, OR, XOR, NOT
<<  >>              // Shift left, shift right
```

### Logical
```cu
&&  ||  !           // AND, OR, NOT
```

### Comparison
```cu
==  !=              // Equality
<  >  <=  >=        // Relational
```

### Assignment
```cu
=                   // Simple assignment
+=  -=  *=  /=  %=  // Compound assignment
&=  |=  ^=          // Bitwise compound
<<=  >>=            // Shift compound
```

### Memory
```cu
&                   // Address-of
*                   // Dereference
->                  // Pointer member access
.                   // Direct member access
[]                  // Array subscript
```

### Other
```cu
?:                  // Ternary conditional
,                   // Comma operator
()                  // Function call / grouping
```

## Operator Precedence

From highest to lowest:

1. `()` `[]` `->` `.` - Postfix
2. `!` `~` `+` `-` `*` `&` `++` `--` `sizeof` `cast` - Unary
3. `*` `/` `%` - Multiplicative
4. `+` `-` - Additive
5. `<<` `>>` - Shift
6. `<` `<=` `>` `>=` - Relational
7. `==` `!=` - Equality
8. `&` - Bitwise AND
9. `^` - Bitwise XOR
10. `|` - Bitwise OR
11. `&&` - Logical AND
12. `||` - Logical OR
13. `?:` - Ternary
14. `=` `+=` `-=` etc. - Assignment
15. `,` - Comma

## Statements

### Expression Statement
```cu
x = 5;
func();
```

### Block Statement
```cu
{
    i32 x = 10;
    x = x + 1;
}
```

### If Statement
```cu
if (condition) {
    // true branch
} else {
    // false branch
}
```

### While Loop
```cu
while (condition) {
    // body
}
```

### For Loop
```cu
for (i32 i = 0; i < 10; i++) {
    // body
}
```

### Return Statement
```cu
return;
return value;
```

### Break/Continue
```cu
break;
continue;
```

### Goto/Label
```cu
goto label;
label:
    statement;
```

## Declarations

### Variable Declaration
```cu
i32 x;              // Uninitialized
i32 y = 42;         // Initialized
const i32 z = 100;  // Constant
static i32 w = 0;   // Static storage
```

### Function Declaration
```cu
fn add(i32 a, i32 b) -> i32 {
    return a + b;
}

// No return value
fn print_hello() -> void {
    // ...
}

// External function
extern fn malloc(u64 size) -> ptr;
```

### Structure Declaration
```cu
struct Point {
    i32 x;
    i32 y;
}

// With methods (syntax sugar for functions)
struct Vector {
    f32 x;
    f32 y;
    f32 z;
}

fn Vector_length(Vector* self) -> f32 {
    // ...
}
```

### Union Declaration
```cu
union Value {
    i32 as_int;
    f32 as_float;
    ptr as_ptr;
}
```

### Enum Declaration
```cu
enum Color {
    RED,
    GREEN,
    BLUE
}

// With explicit values
enum Status {
    OK = 0,
    ERROR = -1,
    PENDING = 1
}
```

## Type Annotations

```cu
// Pointer types
i32* ptr_to_int;
void* generic_ptr;

// Array types
i32[10] fixed_array;
i32[] slice;  // Fat pointer (ptr + length)

// Function pointer types
fn(i32, i32) -> i32 callback;
```

## Attributes

```cu
// Alignment
@align(16)
struct AlignedData {
    f32[4] values;
}

// Packing
@packed
struct PackedStruct {
    u8 a;
    u32 b;
}

// No mangle (preserve symbol name)
@nomangle
fn exported_function() -> void {
    // ...
}

// Inline
@inline
fn fast_function() -> i32 {
    return 42;
}

// Calling convention
@cdecl
@stdcall
@fastcall
fn platform_specific() -> void {
    // ...
}
```

## Preprocessor (Minimal)

CU has a minimal preprocessor for conditional compilation:

```cu
#if defined(PLATFORM_LINUX)
    // Linux-specific code
#elif defined(PLATFORM_WINDOWS)
    // Windows-specific code
#else
    // Default code
#endif

#define MAX_SIZE 1024
#undef MAX_SIZE
```

## Semicolons

- Required after statements
- Not required after block statements (`if`, `while`, `for`, function definitions)
- Required after struct/union/enum definitions
