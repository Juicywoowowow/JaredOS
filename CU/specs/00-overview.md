# CU (C Universal) - Overview

**Version:** 0.1.0  
**Status:** Specification Draft  
**Last Updated:** 2025-12-08

## Introduction

CU (C Universal) is a minimalist, portable gateway language designed to facilitate seamless interoperability between C++, C, and Assembly. It follows a strict philosophy of simplicity and portability, serving as a universal bridge for low-level systems programming.

## Philosophy

### Core Principles

1. **Simplicity First**
   - Minimal syntax and features
   - Easy to learn and understand
   - No hidden complexity or magic

2. **Portability**
   - Works across all major platforms
   - Consistent behavior across C, C++, and Assembly
   - No platform-specific dependencies in core language

3. **Interoperability**
   - Seamless calling conventions with C/C++/Assembly
   - Zero-cost abstractions
   - Direct memory access and control

4. **Explicitness**
   - No implicit conversions
   - Clear ownership and lifetime semantics
   - Predictable behavior

## Design Goals

- **Gateway Language**: Enable higher-level languages to communicate with C/C++/Assembly easily
- **Bidirectional Calling**: C/C++/Assembly can call CU, and CU can call them
- **Minimal Runtime**: No garbage collection, minimal standard library
- **Compile-Time Safety**: Maximum safety without runtime overhead
- **Bare Metal Capable**: Can run without an operating system

## Target Use Cases

1. **Systems Programming**: Operating systems, drivers, embedded systems
2. **Performance-Critical Code**: Game engines, real-time systems
3. **Foreign Function Interface**: Bridge between different language ecosystems
4. **Low-Level Libraries**: Reusable components across C/C++/Assembly projects
5. **Bootloaders and Firmware**: Bare metal programming

## Non-Goals

- **Not a replacement** for C/C++ - it's a complement
- **Not object-oriented** - procedural and data-oriented
- **Not high-level** - stays close to the metal
- **Not feature-rich** - intentionally minimal

## Document Structure

This specification is organized into the following documents:

1. **00-overview.md** (this document) - Introduction and philosophy
2. **01-syntax.md** - Language syntax and grammar
3. **02-types.md** - Type system and data structures
4. **03-memory.md** - Memory model and management
5. **04-interop.md** - Interoperability with C/C++/Assembly
6. **05-abi.md** - Application Binary Interface
7. **06-modules.md** - Module system and organization
8. **07-compilation.md** - Compilation model and toolchain
9. **08-stdlib.md** - Standard library (minimal)
10. **09-examples.md** - Code examples and patterns

## Versioning

CU follows semantic versioning (MAJOR.MINOR.PATCH):
- **MAJOR**: Breaking changes to language or ABI
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes and clarifications

## License

The CU specification is released under the MIT License, allowing free use and implementation.
