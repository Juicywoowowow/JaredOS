/**
 * jaredOS - Shell Implementation
 */

#include "shell.h"
#include "commands.h"
#include "parser.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../lib/printf.h"
#include "../lib/string.h"

/* Command line buffer */
static char line_buffer[SHELL_MAX_LINE];
static int line_pos = 0;

/**
 * Print shell prompt
 */
void shell_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("jaredOS");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("> ");
}

/**
 * Read a line of input
 */
static void read_line(void) {
    line_pos = 0;
    memset(line_buffer, 0, SHELL_MAX_LINE);
    
    while (1) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            vga_putchar('\n');
            return;
        }
        
        if (c == '\b') {
            if (line_pos > 0) {
                line_pos--;
                line_buffer[line_pos] = '\0';
                vga_putchar('\b');
            }
            continue;
        }
        
        if (c >= 32 && c < 127 && line_pos < SHELL_MAX_LINE - 1) {
            line_buffer[line_pos++] = c;
            vga_putchar(c);
        }
    }
}

/**
 * Execute a command line
 */
static void execute_command(void) {
    /* Skip empty lines */
    if (line_pos == 0) return;
    
    /* Parse command line */
    int argc;
    char *argv[SHELL_MAX_ARGS];
    parse_command(line_buffer, &argc, argv, SHELL_MAX_ARGS);
    
    if (argc == 0) return;
    
    /* Execute command */
    if (!commands_execute(argc, argv)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Unknown command: %s\n", argv[0]);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        kprintf("Type 'help' for available commands.\n");
    }
}

/**
 * Main shell loop
 */
void shell_run(void) {
    kprintf("Welcome to the jaredOS shell!\n");
    kprintf("Type 'help' for available commands.\n\n");
    
    while (1) {
        shell_prompt();
        read_line();
        execute_command();
    }
}
