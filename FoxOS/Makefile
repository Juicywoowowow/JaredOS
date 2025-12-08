# =============================================================================
# FoxOS Makefile
# =============================================================================
#
# Build system for FoxOS - A simple graphical operating system
#
# Targets:
#   make all     - Build the bootable OS image
#   make run     - Build and run in QEMU
#   make debug   - Build and run with QEMU debugger
#   make clean   - Remove all build artifacts
#
# Requirements:
#   - i686-elf-gcc (cross-compiler)
#   - nasm (assembler)
#   - qemu-system-i386 (emulator)
#
# DEBUGGING TIPS:
#   - If "command not found": install the missing tool
#   - If linker errors: check that kernel_entry.o is first in OBJS
#   - If boot fails: verify boot sector signature at bytes 510-511
#
# =============================================================================

# Cross-compiler toolchain
# On macOS: brew install i686-elf-gcc nasm qemu
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Compiler flags
CFLAGS = -ffreestanding -m32 -nostdlib -nostdinc -fno-builtin \
         -fno-stack-protector -nostartfiles -nodefaultlibs \
         -Wall -Wextra -c

# Linker flags
LDFLAGS = -T linker.ld -nostdlib

# Assembler flags (NASM)
ASFLAGS_BIN = -f bin
ASFLAGS_ELF = -f elf32

# Directories
SRC_DIR = source
BUILD_DIR = build

# Output files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = foxos.img

# Source files
ASM_SOURCES = $(SRC_DIR)/kernel_entry.asm
C_SOURCES = $(SRC_DIR)/kernelA.c \
            $(SRC_DIR)/kernelB.c \
            $(SRC_DIR)/interrupts.c \
            $(SRC_DIR)/keyboard.c \
            $(SRC_DIR)/mouse.c \
            $(SRC_DIR)/vga.c \
            $(SRC_DIR)/font.c \
            $(SRC_DIR)/window.c \
            $(SRC_DIR)/taskbar.c \
            $(SRC_DIR)/pong.c \
            $(SRC_DIR)/main.c

# Object files (kernel_entry MUST be first!)
OBJS = $(BUILD_DIR)/kernel_entry.o \
       $(BUILD_DIR)/kernelA.o \
       $(BUILD_DIR)/kernelB.o \
       $(BUILD_DIR)/interrupts.o \
       $(BUILD_DIR)/keyboard.o \
       $(BUILD_DIR)/mouse.o \
       $(BUILD_DIR)/vga.o \
       $(BUILD_DIR)/font.o \
       $(BUILD_DIR)/window.o \
       $(BUILD_DIR)/taskbar.o \
       $(BUILD_DIR)/pong.o \
       $(BUILD_DIR)/main.o

# =============================================================================
# Main Targets
# =============================================================================

.PHONY: all run debug clean

all: $(OS_IMAGE)
	@echo "========================================"
	@echo " FoxOS build complete!"
	@echo " Run 'make run' to start QEMU"
	@echo "========================================"

run: $(OS_IMAGE)
	@echo "Starting FoxOS in QEMU..."
	qemu-system-i386 -fda $(OS_IMAGE) -m 32M -debugcon stdio

debug: $(OS_IMAGE)
	@echo "Starting FoxOS in QEMU with GDB server..."
	@echo "Connect with: gdb -ex 'target remote localhost:1234'"
	qemu-system-i386 -fda $(OS_IMAGE) -m 32M -s -S -debugcon stdio

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OS_IMAGE)
	rm -f linker.ld
	@echo "Cleaned!"

# =============================================================================
# Build Rules
# =============================================================================

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate linker script
linker.ld:
	@echo "ENTRY(_start)" > linker.ld
	@echo "SECTIONS {" >> linker.ld
	@echo "    . = 0x10000;" >> linker.ld
	@echo "    .text : { *(.text) }" >> linker.ld
	@echo "    .rodata : { *(.rodata) }" >> linker.ld
	@echo "    .data : { *(.data) }" >> linker.ld
	@echo "    .bss : { *(.bss) }" >> linker.ld
	@echo "}" >> linker.ld

# Assemble boot sector (16-bit, raw binary)
$(BOOT_BIN): $(SRC_DIR)/boot.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS_BIN) $< -o $@
	@echo "Built bootloader"

# Assemble kernel entry (32-bit, ELF)
$(BUILD_DIR)/kernel_entry.o: $(SRC_DIR)/kernel_entry.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS_ELF) $< -o $@

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Link kernel
$(KERNEL_BIN): $(OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Linked kernel"

# Create bootable floppy image
# Boot sector (512 bytes) + Kernel (padded to 32 sectors = 16KB)
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "Creating bootable image..."
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
	@# Pad to at least 32 sectors (16KB) for floppy
	@truncate -s 32768 $(OS_IMAGE) 2>/dev/null || \
	    dd if=/dev/zero bs=1 count=$$(( 32768 - $$(stat -f%z $(OS_IMAGE) 2>/dev/null || stat -c%s $(OS_IMAGE)) )) >> $(OS_IMAGE) 2>/dev/null || true
	@echo "Created $(OS_IMAGE)"

# =============================================================================
# Help
# =============================================================================

help:
	@echo "FoxOS Build System"
	@echo "=================="
	@echo ""
	@echo "Targets:"
	@echo "  make all    - Build foxos.img"
	@echo "  make run    - Build and run in QEMU"
	@echo "  make debug  - Build and run with GDB server"
	@echo "  make clean  - Remove build artifacts"
	@echo ""
	@echo "Requirements:"
	@echo "  i686-elf-gcc, nasm, qemu-system-i386"
	@echo ""
	@echo "On macOS:"
	@echo "  brew tap nativeos/i386-elf-toolchain"
	@echo "  brew install i386-elf-gcc nasm qemu"
