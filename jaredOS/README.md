# jaredOS

**jaredOS** is a custom 32-bit operating system written in C and Assembly from scratch. It features a custom bootloader, kernel, shell, filesystem, text editor, and its own JIT-compiled programming language called **Gwango**.

![jaredOS](https://img.shields.io/badge/jaredOS-v0.2.0-blue) ![Arch](https://img.shields.io/badge/arch-x86-orange) ![Lang](https://img.shields.io/badge/lang-C%2FASM-green)

## 🚀 Features

### Core System
- **Custom Bootloader**: 2-stage bootloader (Assembly) loading 32-bit protected mode kernel.
- **Kernel**: Monolithic kernel with GDT, IDT, ISR, and IRQ handling.
- **Memory Management**: Physical Memory Manager (PMM) and Virtual Memory Manager (VMM).
- **Drivers**: VGA Text Mode (80x25), PS/2 Keyboard, PIT Timer, Serial Port, ATA/IDE Disk.

### User Space & Shell
- **Shell**: Interactive command-line interface with 15+ commands.
- **Filesystem**: Simple flat filesystem (SimpleFS) with persistent storage on ATA disk.
- **Text Editor**: Full-screen editor (`edit`) with syntax highlighting, save/load support.
- **Utilities**: `ls`, `cat`, `write`, `format`, `mem`, `dump`, `calc`, `time`.

### ⚡ Gwango Language
A custom programming language built for jaredOS with **real x86 JIT compilation**.

- **JIT Compiler**: Compiles source code directly to x86 machine code in memory.
- **Disassembler**: Inspect generated assembly with `gwan -d`.
- **Syntax**: Assembly/HolyC-inspired syntax.
- **Kernel Bindings**: Direct access to VGA, keyboard, and filesystem.

**Example Code:**
```asm
; Hello World in Gwango
var x = 10
@vga.print "Hello from Gwango!"
@vga.newline

loop i = 1 to 5
    @vga.print i
end
```

---

## 🛠️ Build & Run

### Prerequisites
- `nasm`
- `i686-elf-gcc` (cross-compiler)
- `qemu-system-i386`
- `make`

### Quick Start
```bash
# Build and run in QEMU
make run

# Clean build
make clean
```

This will:
1. Compile the kernel and bootloader.
2. Create `jaredOS.img` (floppy boot image).
3. Create `disk.img` (1MB hard disk image).
4. Launch QEMU with both drives attached.

---

## 📖 Usage Guide

### Shell Commands
| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `clear` | Clear screen |
| `ls` | List files on disk |
| `format` | Format the disk (required first time) |
| `edit <file>` | Open text editor |
| `gwan <file>` | Run Gwango script |
| `mem` | Show memory usage |
| `dump <addr>` | Hex dump memory |
| `reboot` | Reboot system |

### Text Editor (`edit`)
- **Ctrl+S**: Save file to disk
- **Ctrl+Q**: Quit editor
- Supports basic navigation and editing.

### Gwango Language (`gwan`)
Run the REPL by typing `gwan`, or execute a file with `gwan filename.gw`.

**Flags:**
- `-d`: Dump generated x86 assembly (e.g., `gwan -d test.gw`)

**Language Reference:**
```asm
; Variables
var x = 42
var y = x + 10

; Kernel Calls
@vga.print "Value: "
@vga.print y
@vga.clear
@vga.newline

; Control Flow
if x > 20
    @vga.print "Big"
else
    @vga.print "Small"
end

; Loops
loop i = 0 to 10
    @vga.print i
end
```

---

## 📂 Project Structure

```
jaredOS/
├── boot/           # Bootloader (Stage 1 & 2)
├── kernel/
│   ├── core/       # GDT, IDT, ISR, IRQ
│   ├── drivers/    # VGA, Keyboard, ATA, Timer
│   ├── fs/         # SimpleFS implementation
│   ├── memory/     # PMM, VMM, Heap
│   ├── shell/      # Shell, Editor, Commands
│   └── lib/        # Standard library (string, stdlib)
├── lang/
│   └── gwango/     # Gwango Language
│       └── core/   # Lexer, Parser, JIT, Runtime
└── Makefile        # Build system
```

---

## 📜 License
MIT License. Created by **Alexander Briant Tadiosa**.
