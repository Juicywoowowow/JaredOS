# VBox - Simple x86 Emulator

A lightweight QEMU-inspired real-mode x86 emulator with SDL2 graphics output.

## Features

- Real-mode x86 emulation (16-bit, 1MB address space)
- SDL2 graphical VGA output
- BIOS interrupt support (INT 10h, INT 13h, INT 16h)
- Simple CLI to run `.bin` files

## Building

```bash
make
```

## Usage

```bash
./vbox [options] <binary.bin>

Options:
  -m <size>   Memory size in KB (default: 640)
  -d          Enable debug mode
  -v          Verbose output
```

## Project Structure

```
vbox/
├── include/vbox/    # Header files
├── src/
│   ├── cpu/         # CPU emulation
│   ├── memory/      # Memory subsystem
│   ├── bios/        # BIOS interrupts
│   ├── devices/     # Hardware emulation
│   └── loader/      # Binary loaders
├── docs/            # Documentation
└── examples/        # Example programs
```

## License

MIT License
