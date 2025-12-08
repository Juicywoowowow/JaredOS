/*
 * =============================================================================
 * SharkOS Kernel Main (kernel.c)
 * =============================================================================
 * Entry point for the C kernel. Called by kernel_entry.asm after the
 * bootloader has set up protected mode.
 *
 * Boot sequence:
 *   1. BIOS
 *   2. boot.asm (stage 1 - loads stage 2)
 *   3. boot_stage2.asm (enables A20, GDT, protected mode, loads kernel)
 *   4. kernel_entry.asm (calls kernel_main)
 *   5. kernel_main() <-- YOU ARE HERE
 *
 * DEBUGGING TIPS:
 *   - If kernel doesn't start, verify it's loaded at 0x1000
 *   - Check that protected mode switch worked (PM OK printed)
 *   - Use vga_print_hex() to dump memory/register values
 *   - If stuck, print debug messages at each initialization step
 *   - Triple fault (reboot) usually means stack corruption or bad pointer
 * =============================================================================
 */

#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "memory.h"
#include "string.h"
#include "fs.h"

/* ----------------------------------------------------------------------------
 * kernel_panic - Handle unrecoverable errors
 *
 * Prints an error message in red and halts the CPU forever.
 * This should only be called for truly fatal errors where
 * continuing execution would cause more damage.
 *
 * Examples of when to panic:
 *   - Memory allocation failed during init
 *   - Critical hardware not responding
 *   - Data structures corrupted
 *   - Stack overflow detected
 * ---------------------------------------------------------------------------- */
void kernel_panic(const char* message) {
    /* Disable interrupts immediately */
    __asm__ volatile("cli");
    
    /* Set scary colors */
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    
    /* Clear screen with red background */
    uint16_t* vga = (uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (0x4F << 8) | ' ';
    }
    
    /* Reset cursor */
    vga_set_cursor(0, 0);
    
    /* Print panic message */
    vga_print("\n\n");
    vga_print("  ==================================================\n");
    vga_print("                   KERNEL PANIC!                    \n");
    vga_print("  ==================================================\n\n");
    vga_print("  Error: ");
    vga_print(message);
    vga_print("\n\n");
    vga_print("  The system has been halted to prevent damage.\n");
    vga_print("  Please restart your computer.\n\n");
    vga_print("  ==================================================\n");
    
    /* Halt forever */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* ----------------------------------------------------------------------------
 * kernel_main - Kernel entry point
 *
 * This is called by kernel_entry.asm after protected mode is set up.
 * Initializes all subsystems and launches the shell.
 * Should never return!
 * ---------------------------------------------------------------------------- */
void kernel_main(void) {
    /* -------------------------------------------------------------------------
     * Step 1: Initialize VGA driver
     * This gives us the ability to print to screen - essential for debugging!
     * ------------------------------------------------------------------------- */
    vga_init();
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("============================================\n");
    vga_print("           SharkOS Kernel Starting          \n");
    vga_print("============================================\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* -------------------------------------------------------------------------
     * Step 2: Print boot status messages
     * Helps with debugging - you can see how far the kernel gets
     * ------------------------------------------------------------------------- */
    vga_print("[");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print(" OK ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("] VGA text mode initialized (80x25, 16 colors)\n");
    
    /* -------------------------------------------------------------------------
     * Step 3: Initialize keyboard driver
     * Required for shell input
     * ------------------------------------------------------------------------- */
    keyboard_init();
    
    vga_print("[");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print(" OK ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("] Keyboard driver initialized (PS/2, US layout)\n");
    
    /* -------------------------------------------------------------------------
     * Step 4: Initialize Filesystem
     * ------------------------------------------------------------------------- */
    fs_init();
    
    /* -------------------------------------------------------------------------
     * Step 4: Print system information
     * ------------------------------------------------------------------------- */
    vga_print("[");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("INFO");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("] Running in 32-bit protected mode\n");
    
    vga_print("[");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("INFO");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("] Kernel loaded at address 0x1000\n");
    
    /* -------------------------------------------------------------------------
     * Step 5: Launch shell
     * This should never return. If it does, we panic.
     * ------------------------------------------------------------------------- */
    vga_print("[");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print(" OK ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("] Starting shell...\n");
    
    shell_run();
    
    /* -------------------------------------------------------------------------
     * If we get here, something went very wrong
     * ------------------------------------------------------------------------- */
    kernel_panic("Shell returned unexpectedly!");
}
