# ==============================================================================
# SharkOS Makefile
# ==============================================================================
# Build system for SharkOS. Creates a bootable 1.44MB floppy disk image.
#
# Prerequisites:
#   - i686-elf-gcc (cross-compiler) - brew install i686-elf-gcc
#   - nasm (assembler) - brew install nasm
#   - qemu (emulator) - brew install qemu
#
# Usage:
#   make        - Build shark.img
#   make run    - Build and run in QEMU
#   make debug  - Build and run with QEMU debug mode
#   make clean  - Remove build artifacts
#
# DEBUGGING TIPS:
#   - If 'i686-elf-gcc not found', check your PATH or use full path
#   - If QEMU shows black screen, bootloader may have failed
#   - Use 'make debug' to see QEMU debug output
#   - Check file sizes: boot.bin must be 512 bytes exactly
# ==============================================================================

# Toolchain
AS = nasm
CC = i686-elf-gcc
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

# Flags
ASFLAGS = -f elf32
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -c
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
SRC_DIR = src
BOOT_DIR = $(SRC_DIR)/boot
BUILD_DIR = build

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
C_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Output
OUTPUT = shark.img

# ==============================================================================
# Main targets
# ==============================================================================

.PHONY: all run debug clean

all: $(OUTPUT)

# Run in QEMU
run: $(OUTPUT)
	@echo "[QEMU] Starting SharkOS..."
	qemu-system-i386 -fda $(OUTPUT) -boot a

# Run with debug output
debug: $(OUTPUT)
	@echo "[QEMU] Starting SharkOS (debug mode)..."
	qemu-system-i386 -fda $(OUTPUT) -boot a -d int,cpu_reset -no-reboot

# Clean build artifacts
clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(OUTPUT)

# ==============================================================================
# Build process
# ==============================================================================

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Assemble boot sector (stage 1)
$(BUILD_DIR)/boot.bin: $(BOOT_DIR)/boot.asm | $(BUILD_DIR)
	@echo "[ASM] $<"
	$(AS) -f bin $< -o $@

# Assemble stage 2 bootloader
$(BUILD_DIR)/boot_stage2.bin: $(BOOT_DIR)/boot_stage2.asm | $(BUILD_DIR)
	@echo "[ASM] $<"
	$(AS) -f bin $< -o $@

# Assemble kernel entry
$(BUILD_DIR)/kernel_entry.o: $(SRC_DIR)/kernel_entry.asm | $(BUILD_DIR)
	@echo "[ASM] $<"
	$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) $< -o $@

# Link kernel
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel_entry.o $(C_OBJECTS)
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/kernel.elf $^
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@

# Create floppy image
# Layout:
#   Sector 1: boot.bin (512 bytes) - Stage 1 bootloader
#   Sectors 2-5: boot_stage2.bin (2KB) - Stage 2 bootloader
#   Sectors 6+: kernel.bin - Kernel
#   Total: 1.44MB (2880 sectors * 512 bytes)
$(OUTPUT): $(BUILD_DIR)/boot.bin $(BUILD_DIR)/boot_stage2.bin $(BUILD_DIR)/kernel.bin
	@echo "[IMG] Creating floppy image..."
	
	# Create empty 1.44MB floppy image
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	
	# Write boot sector to first sector
	dd if=$(BUILD_DIR)/boot.bin of=$@ conv=notrunc bs=512 count=1 2>/dev/null
	
	# Write stage 2 starting at sector 2 (offset 512)
	dd if=$(BUILD_DIR)/boot_stage2.bin of=$@ conv=notrunc bs=512 seek=1 2>/dev/null
	
	# Write kernel starting at sector 6 (offset 2560)
	dd if=$(BUILD_DIR)/kernel.bin of=$@ conv=notrunc bs=512 seek=5 2>/dev/null
	
	@echo "[DONE] Created $(OUTPUT) successfully!"
	@echo ""
	@echo "  Run 'make run' to start SharkOS in QEMU"
	@echo ""

# ==============================================================================
# Debug helpers
# ==============================================================================

# Show file sizes
sizes: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/boot_stage2.bin $(BUILD_DIR)/kernel.bin
	@echo "=== Build artifact sizes ==="
	@ls -la $(BUILD_DIR)/boot.bin
	@ls -la $(BUILD_DIR)/boot_stage2.bin
	@ls -la $(BUILD_DIR)/kernel.bin

# Disassemble kernel
disasm: $(BUILD_DIR)/kernel.elf
	objdump -d $(BUILD_DIR)/kernel.elf | less

# Show kernel symbols
symbols: $(BUILD_DIR)/kernel.elf
	objdump -t $(BUILD_DIR)/kernel.elf
