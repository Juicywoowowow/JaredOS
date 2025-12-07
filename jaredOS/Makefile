# =============================================================================
# jaredOS Makefile
# =============================================================================

# Toolchain
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-stack-protector \
         -nostdlib -Ikernel -Iinclude
ASFLAGS_BIN = -f bin
ASFLAGS_ELF = -f elf32
LDFLAGS = -T linker.ld -nostdlib

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
BUILD_DIR = build

# Source files
BOOT_STAGE1 = $(BOOT_DIR)/stage1.asm
BOOT_STAGE2 = $(BOOT_DIR)/stage2.asm

# Kernel C sources
KERNEL_C_SRCS = $(KERNEL_DIR)/main.c \
                $(KERNEL_DIR)/core/gdt.c \
                $(KERNEL_DIR)/core/idt.c \
                $(KERNEL_DIR)/core/isr.c \
                $(KERNEL_DIR)/core/irq.c \
                $(KERNEL_DIR)/memory/pmm.c \
                $(KERNEL_DIR)/memory/vmm.c \
                $(KERNEL_DIR)/memory/heap.c \
                $(KERNEL_DIR)/drivers/vga.c \
                $(KERNEL_DIR)/drivers/keyboard.c \
                $(KERNEL_DIR)/drivers/timer.c \
                $(KERNEL_DIR)/drivers/serial.c \
                $(KERNEL_DIR)/drivers/ata.c \
                $(KERNEL_DIR)/lib/string.c \
                $(KERNEL_DIR)/lib/stdlib.c \
                $(KERNEL_DIR)/lib/printf.c \
                $(KERNEL_DIR)/fs/simplefs.c \
                $(KERNEL_DIR)/shell/shell.c \
                $(KERNEL_DIR)/shell/commands.c \
                $(KERNEL_DIR)/shell/calc.c \
                $(KERNEL_DIR)/shell/parser.c \
                $(KERNEL_DIR)/shell/editor.c

# Kernel ASM sources
KERNEL_ASM_SRCS = $(KERNEL_DIR)/core/gdt_asm.asm \
                  $(KERNEL_DIR)/core/idt_asm.asm \
                  $(KERNEL_DIR)/core/isr_asm.asm \
                  $(KERNEL_DIR)/core/irq_asm.asm

# Object files
KERNEL_C_OBJS = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_C_SRCS))
KERNEL_ASM_OBJS = $(patsubst $(KERNEL_DIR)/%.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRCS))
KERNEL_OBJS = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

# Output files
STAGE1_BIN = $(BUILD_DIR)/stage1.bin
STAGE2_BIN = $(BUILD_DIR)/stage2.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
FLOPPY_IMG = jaredOS.img

# Floppy size (1.44 MB)
FLOPPY_SIZE = 1474560

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean run dirs

all: dirs $(FLOPPY_IMG)

# Create floppy image
$(FLOPPY_IMG): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "Creating floppy image..."
	dd if=/dev/zero of=$(FLOPPY_IMG) bs=512 count=2880 2>/dev/null
	dd if=$(STAGE1_BIN) of=$(FLOPPY_IMG) bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(STAGE2_BIN) of=$(FLOPPY_IMG) bs=512 seek=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$(FLOPPY_IMG) bs=512 seek=5 conv=notrunc 2>/dev/null
	@echo "Created $(FLOPPY_IMG)"

# Build bootloader stage 1
$(STAGE1_BIN): $(BOOT_STAGE1) | dirs
	@echo "Assembling stage1..."
	$(AS) $(ASFLAGS_BIN) $< -o $@

# Build bootloader stage 2
$(STAGE2_BIN): $(BOOT_STAGE2) | dirs
	@echo "Assembling stage2..."
	$(AS) $(ASFLAGS_BIN) $< -o $@

# Link kernel
$(KERNEL_BIN): $(KERNEL_OBJS) | dirs
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/kernel.elf $(KERNEL_OBJS)
	i686-elf-objcopy -O binary $(BUILD_DIR)/kernel.elf $(KERNEL_BIN)

# Compile C sources
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | dirs
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM sources (ELF format for linking)
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm | dirs
	@mkdir -p $(dir $@)
	@echo "Assembling $<..."
	$(AS) $(ASFLAGS_ELF) $< -o $@

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)/core
	@mkdir -p $(BUILD_DIR)/memory
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/lib
	@mkdir -p $(BUILD_DIR)/fs
	@mkdir -p $(BUILD_DIR)/shell

# Create disk image for filesystem (1MB)
disk.img:
	dd if=/dev/zero of=disk.img bs=1024 count=1024 2>/dev/null
	@echo "Created disk.img (1MB)"

# Run in QEMU with disk
run: $(FLOPPY_IMG) disk.img
	qemu-system-i386 -drive file=$(FLOPPY_IMG),format=raw,if=floppy -drive file=disk.img,format=raw,if=ide -boot a

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(FLOPPY_IMG)
	rm -f disk.img
