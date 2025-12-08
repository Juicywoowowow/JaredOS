# SharkOS 🦈

A simple x86 operating system written in C and Assembly with a custom bootloader.

## Features

- **Custom 2-stage bootloader** - Written in x86 assembly
- **32-bit protected mode kernel** - Full access to 4GB memory
- **VGA text mode driver** - 80x25 display with 16 colors
- **PS/2 keyboard driver** - Full US layout support with shift
- **Interactive shell** - Command-line interface with built-in commands

### Shell Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `clear` | Clear the screen |
| `echo <text>` | Print text to screen |
| `version` | Show OS version info |
| `reboot` | Reboot the system |
| `shutdown` | Halt the CPU |
| `calc (x op y)` | Calculator: `calc (5 + 3)`, `calc (10 * 2)` |
| `colors [1-15]` | Show VGA color palette |

## Requirements

Install via Homebrew (macOS):

```bash
brew install i686-elf-gcc nasm qemu
```

## Building

```bash
cd SharkOS

# Build the OS
make

# Build and run in QEMU
make run

# Run with debug output
make debug

# Clean build artifacts
make clean
```

## Project Structure

```
SharkOS/
├── Makefile              # Build system
├── linker.ld             # Kernel linker script
├── README.md             # This file
└── src/
    ├── boot/
    │   ├── boot.asm      # Stage 1 bootloader (MBR)
    │   └── boot_stage2.asm # Stage 2 bootloader
    ├── kernel_entry.asm  # Kernel entry point
    ├── kernel.c/h        # Kernel main
    ├── vga.c/h           # VGA text driver
    ├── keyboard.c/h      # Keyboard driver
    ├── shell.c/h         # Shell implementation
    ├── string.c/h        # String utilities
    ├── memory.c/h        # Memory utilities
    ├── io.h              # Port I/O functions
    └── types.h           # Type definitions
```

## Boot Sequence

```
BIOS → boot.asm (512B) → boot_stage2.asm → Protected Mode → kernel.c → shell
```

1. **BIOS** loads first 512 bytes from floppy to 0x7C00
2. **Stage 1** (boot.asm) sets up segments, loads stage 2
3. **Stage 2** enables A20 line, sets up GDT, switches to protected mode
4. **Kernel entry** calls `kernel_main()` in C
5. **Shell** starts and waits for user input

## Calculator Examples

```
shark> calc (5 + 5)
= 10

shark> calc (7 * 8)
= 56

shark> calc (100 / 0)
Error: Division by zero!
```

## Debugging Tips

- **Black screen?** Check boot.bin is exactly 512 bytes
- **Stuck at "PM OK"?** Kernel may not be at 0x1000
- **No keyboard?** Make sure you're focusing the QEMU window
- **Triple fault?** Likely stack corruption or null pointer

Use debug mode for more info:
```bash
make debug
```

## License

Educational/personal use. Feel free to learn from and modify!

---

*SharkOS - Because every OS needs more sharks* 🦈
