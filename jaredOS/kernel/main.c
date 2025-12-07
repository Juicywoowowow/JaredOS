/**
 * jaredOS - Kernel Main Entry Point
 */

#include "types.h"
#include "core/gdt.h"
#include "core/idt.h"
#include "core/isr.h"
#include "core/irq.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "drivers/serial.h"
#include "drivers/ata.h"
#include "fs/simplefs.h"
#include "lib/printf.h"
#include "shell/shell.h"

/**
 * Kernel entry point - called from bootloader
 */
void kernel_main(void) {
    /* Initialize VGA first for output */
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_clear();

    /* Print banner */
    kprintf("\n");
    kprintf("  =====================================\n");
    kprintf("     _                   _  ___  ____  \n");
    kprintf("    (_) __ _ _ __ ___  __| |/ _ \\/ ___| \n");
    kprintf("    | |/ _` | '__/ _ \\/ _` | | | \\___ \\ \n");
    kprintf("    | | (_| | | |  __/ (_| | |_| |___) |\n");
    kprintf("   _/ |\\__,_|_|  \\___|\\__,_|\\___/|____/ \n");
    kprintf("  |__/                                  \n");
    kprintf("  =====================================\n");
    kprintf("         Version 0.2.0\n\n");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Initialize system components */
    kprintf("[INIT] Global Descriptor Table...\n");
    gdt_init();

    kprintf("[INIT] Interrupt Descriptor Table...\n");
    idt_init();

    kprintf("[INIT] Interrupt Service Routines...\n");
    isr_init();

    kprintf("[INIT] Hardware Interrupts...\n");
    irq_init();

    kprintf("[INIT] Programmable Interval Timer...\n");
    timer_init(100);  /* 100 Hz */

    kprintf("[INIT] Keyboard Driver...\n");
    keyboard_init();

    kprintf("[INIT] Serial Port (COM1)...\n");
    serial_init();

    /* Initialize disk */
    kprintf("[INIT] ATA/IDE Driver...\n");
    if (ata_init()) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("       Disk detected!\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        kprintf("[INIT] Filesystem...\n");
        if (fs_init()) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            kprintf("       Filesystem ready.\n");
        } else {
            vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
            kprintf("       No filesystem. Use 'format' command.\n");
        }
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        kprintf("       No disk detected.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }

    /* Enable interrupts */
    __asm__ volatile ("sti");

    kprintf("\n[OK] System initialized successfully!\n\n");

    /* Start shell */
    shell_run();

    /* Should never reach here */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
