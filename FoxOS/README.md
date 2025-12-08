# 🦊 FoxOS

A simple graphical operating system written in pure C with minimal assembly.
Built for educational purposes and fun experimentation.

![FoxOS Screenshot](docs/screenshot.png)

## ✨ Features

- **32-bit Protected Mode** - Full i686 (x86) support
- **VGA Graphics** - 320x200 resolution with 256 colors
- **Mouse Support** - PS/2 mouse driver with cursor
- **Keyboard Support** - Full PS/2 keyboard with input buffering
- **Window Manager** - Draggable windows with title bars
- **Taskbar** - Start button and system clock
- **Pong Game** - Classic game to test graphics


```
FoxOS/
├── source/
│   ├── boot.asm          # 512-byte bootloader
│   ├── kernel_entry.asm  # Kernel entry point, ISR stubs
│   ├── kernelA.c         # GDT, IDT, PIC, system init
│   ├── kernelB.c         # Memory management, heap
│   ├── interrupts.c      # Timer, IRQ handlers
│   ├── keyboard.c        # PS/2 keyboard driver
│   ├── mouse.c           # PS/2 mouse driver
│   ├── vga.c             # VGA Mode 13h graphics
│   ├── font.c            # 8x8 bitmap font
│   ├── window.c          # Window manager
│   ├── taskbar.c         # Taskbar implementation
│   ├── pong.c            # Pong game
│   ├── main.c            # Main kernel loop
│   └── types.h           # Common types and utilities
├── Makefile
└── README.md
```

## 🛠️ Prerequisites

You need a cross-compiler for i686-elf. On macOS:

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Add the cross-compiler tap
brew tap nativeos/i386-elf-toolchain

# Install required tools
brew install i386-elf-gcc nasm qemu
```

On Linux (Ubuntu/Debian):
```bash
sudo apt-get install gcc-multilib nasm qemu-system-x86
# Note: You may need to build i686-elf-gcc from source
```

## 🚀 Building & Running

```bash
# Build the OS image
make all

# Run in QEMU
make run

# Run with debugger (GDB)
make debug

# Clean build artifacts
make clean
```

## 🎮 Controls

### In the OS
- **Mouse** - Move cursor, click to interact
- **Click window title** - Drag window
- **Click X button** - Close window
- **Click Start** - Toggle start menu

### Pong Game
- **W/S** - Move left paddle
- **I/K** - Move right paddle
- **Space** - Start/pause game
- **R** - Reset game

## 🏗️ Architecture

### Memory Map
```
0x00007C00 - 0x00007DFF : Bootloader (512 bytes)
0x00010000 - 0x0001FFFF : Kernel code and data
0x000A0000 - 0x000BFFFF : VGA video memory
0x00200000 - 0x011FFFFF : Kernel heap (16 MB)
```

### The "Brain" (Core Kernel)

**kernelA.c** - System initialization
- GDT (Global Descriptor Table) setup
- IDT (Interrupt Descriptor Table) setup  
- PIC (Programmable Interrupt Controller) remapping
- Exception and IRQ handlers

**kernelB.c** - Memory management
- Physical page frame allocator (bitmap-based)
- Kernel heap allocator (kmalloc/kfree)
- Memory statistics utilities

### Interrupt Handling
| IRQ | Vector | Handler       |
|-----|--------|---------------|
| 0   | 0x20   | Timer (100Hz) |
| 1   | 0x21   | Keyboard      |
| 12  | 0x2C   | Mouse         |

## 🐛 Debugging Tips

### QEMU Debug Output
Run with debug console output:
```bash
qemu-system-i386 -fda foxos.img -debugcon stdio
```

### Common Issues

**Screen stays black:**
- Check that VGA Mode 13h is set in bootloader
- Verify the boot sector signature (0xAA55) at bytes 510-511

**Triple fault on boot:**
- GDT or IDT setup is incorrect
- Stack pointer may be invalid

**Keyboard/mouse not working:**
- Check that interrupts are enabled (sti)
- Verify PIC remapping is correct
- Check IRQ handlers are installed

**Graphics glitches:**
- Make sure double buffering is used
- Check bounds in drawing functions

### Using GDB
```bash
# Start QEMU with GDB server
make debug

# In another terminal
gdb
(gdb) target remote localhost:1234
(gdb) set architecture i386
(gdb) break kmain
(gdb) continue
```

## 📚 Learning Resources

- [OSDev Wiki](https://wiki.osdev.org/) - Essential OS development resource
- [James Molloy's Kernel Tutorial](http://jamesmolloy.co.uk/tutorial_html/)
- [Bran's Kernel Development](http://www.osdever.net/bkerndev/)
- [Intel x86 Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## 📄 License

This project is released under the MIT License. Feel free to use, modify, and distribute.

## 🦊 Why "FoxOS"?

Because foxes are clever, quick, and just a bit mischievous - just like this little OS!

---

*Built with ❤️ for learning and experimentation*
