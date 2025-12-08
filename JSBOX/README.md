# JSBOX

A lightweight JavaScript engine written in pure C, focusing on excellent error diagnostics and complete VM isolation.

## Features

- 🚀 **Basic JS Execution** - Variables, functions, objects, arrays, control flow, closures
- 📝 **Rich Error Diagnostics** - Source-mapped errors with context, line highlighting, suggestions
- 🔒 **Isolated VM** - In-memory sandboxed filesystem, fake environment
- 🛠️ **CLI Tool** - `jbox` command with VM introspection flags

## Building

```bash
make           # Build jbox
make debug     # Build with debug symbols
make test      # Run test suite
make clean     # Clean build artifacts
```

## Usage

```bash
# Run a JS file
jbox script.js

# Interactive REPL
jbox

# VM inspection
jbox --show-ast script.js
jbox --show-tokens script.js
jbox --trace script.js
```

## Project Structure

```
src/
├── base/        # Foundation utilities (memory, strings, hashmap)
├── diagnostics/ # Error/warning system
├── parsing/     # Lexer, parser, AST
├── runtime/     # VM, interpreter, values
├── builtins/    # console, Math, String, Array
├── isolation/   # Sandbox, virtual filesystem
├── gc/          # Garbage collection
└── cli/         # jbox command-line tool
```

## License

MIT
