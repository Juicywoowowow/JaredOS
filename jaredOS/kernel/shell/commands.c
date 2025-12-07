/**
 * jaredOS - Commands Implementation
 */

#include "commands.h"
#include "calc.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../lib/printf.h"
#include "../lib/string.h"

/* Command structure */
typedef struct {
    const char *name;
    const char *description;
    void (*handler)(int argc, char *argv[]);
} command_t;

/* Command table */
static command_t commands[] = {
    {"help",   "Show available commands",  cmd_help},
    {"clear",  "Clear the screen",         cmd_clear},
    {"echo",   "Print text to screen",     cmd_echo},
    {"about",  "About jaredOS",            cmd_about},
    {"time",   "Show system uptime",       cmd_time},
    {"calc",   "Simple calculator",        cmd_calc},
    {"reboot", "Reboot the system",        cmd_reboot},
    {NULL, NULL, NULL}
};

/**
 * Execute a command
 */
bool commands_execute(int argc, char *argv[]) {
    if (argc == 0) return true;
    
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return true;
        }
    }
    
    return false;
}

/**
 * Help command
 */
void cmd_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    kprintf("\nAvailable commands:\n");
    kprintf("-------------------\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("  %s", commands[i].name);
        /* Pad to 10 characters */
        int len = strlen(commands[i].name);
        for (int j = len; j < 10; j++) vga_putchar(' ');
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        kprintf("- %s\n", commands[i].description);
    }
    
    kprintf("\nUsage examples:\n");
    kprintf("  echo Hello World\n");
    kprintf("  calc 10 + 5\n\n");
}

/**
 * Clear command
 */
void cmd_clear(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    vga_clear();
}

/**
 * Echo command
 */
void cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        kprintf("%s", argv[i]);
        if (i < argc - 1) kprintf(" ");
    }
    kprintf("\n");
}

/**
 * About command
 */
void cmd_about(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("\n  =====================================\n");
    kprintf("    jaredOS v0.1.0\n");
    kprintf("  =====================================\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("\n  A simple TempleOS-inspired operating\n");
    kprintf("  system written in C and Assembly.\n\n");
    kprintf("  Features:\n");
    kprintf("    - Custom 2-stage bootloader\n");
    kprintf("    - VGA text mode (80x25)\n");
    kprintf("    - PS/2 keyboard support\n");
    kprintf("    - Simple shell interface\n\n");
}

/**
 * Time command
 */
void cmd_time(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    uint32_t uptime = timer_get_uptime();
    uint32_t hours = uptime / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    uint32_t seconds = uptime % 60;
    
    kprintf("System uptime: %u:", hours);
    if (minutes < 10) kprintf("0");
    kprintf("%u:", minutes);
    if (seconds < 10) kprintf("0");
    kprintf("%u\n", seconds);
    kprintf("Total ticks: %u\n", timer_get_ticks());
}

/**
 * Calc command (delegates to calc.c)
 */
void cmd_calc(int argc, char *argv[]) {
    calc_execute(argc, argv);
}

/**
 * Reboot command
 */
void cmd_reboot(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    kprintf("Rebooting...\n");
    
    /* Triple fault to reboot */
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
    
    /* Halt if reboot fails */
    __asm__ volatile ("cli; hlt");
}
