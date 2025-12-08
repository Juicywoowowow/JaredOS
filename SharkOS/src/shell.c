/*
 * =============================================================================
 * SharkOS Shell (shell.c)
 * =============================================================================
 * Implementation of the command-line shell.
 *
 * Supported commands:
 *   help     - Show available commands
 *   clear    - Clear the screen
 *   echo     - Print text to screen
 *   version  - Show OS version information
 *   reboot   - Reboot the system
 *   shutdown - Halt the CPU
 *   calc     - Simple calculator: calc (5 + 3)
 *   colors   - Show text in different colors (1-15)
 *
 * DEBUGGING TIPS:
 *   - Command not found? Check strcmp() result and command spelling
 *   - Arguments wrong? Print argc and each argv[] to diagnose
 *   - If shell hangs, keyboard driver might be stuck
 * =============================================================================
 */

#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "memory.h"
#include "io.h"

/* ----------------------------------------------------------------------------
 * OS Version Information
 * ---------------------------------------------------------------------------- */
#define SHARKOS_VERSION     "0.1.0"
#define SHARKOS_CODENAME    "Hammerhead"

/* ----------------------------------------------------------------------------
 * Command Table Entry
 * ---------------------------------------------------------------------------- */
typedef void (*cmd_func_t)(int argc, char* argv[]);

typedef struct {
    const char* name;
    const char* description;
    cmd_func_t func;
} command_t;

/* ----------------------------------------------------------------------------
 * Command Table
 * Add new commands here!
 * ---------------------------------------------------------------------------- */
static const command_t commands[] = {
    {"help",     "Show available commands",           cmd_help},
    {"clear",    "Clear the screen",                  cmd_clear},
    {"echo",     "Print text to screen",              cmd_echo},
    {"version",  "Show OS version",                   cmd_version},
    {"reboot",   "Reboot the system",                 cmd_reboot},
    {"shutdown", "Halt the CPU",                      cmd_shutdown},
    {"calc",     "Calculator: calc (5 + 3)",          cmd_calc},
    {"colors",   "Show colors: colors [1-15]",        cmd_colors},
    {NULL, NULL, NULL}  /* Terminator */
};

/* ----------------------------------------------------------------------------
 * Helper: Parse command line into argc/argv
 *
 * Tokenizes the input line by spaces. Modifies the input line!
 * Returns the number of tokens (argc).
 * ---------------------------------------------------------------------------- */
static int parse_arguments(char* line, char* argv[], int max_args) {
    int argc = 0;
    char* token = line;
    bool in_token = false;
    
    while (*line && argc < max_args) {
        if (*line == ' ' || *line == '\t') {
            if (in_token) {
                *line = '\0';  /* Terminate current token */
                in_token = false;
            }
        } else {
            if (!in_token) {
                argv[argc++] = line;  /* Start new token */
                in_token = true;
            }
        }
        line++;
    }
    
    return argc;
}

/* ----------------------------------------------------------------------------
 * Calculator Helper: Parse and evaluate expression
 *
 * Format: (operand1 operator operand2)
 * Operators: + - * /
 *
 * Returns result, sets *error if something went wrong.
 * Error codes:
 *   0 = success
 *   1 = invalid format
 *   2 = division by zero
 *   3 = unknown operator
 * ---------------------------------------------------------------------------- */
static int calc_evaluate(const char* expr, int* error) {
    int a = 0, b = 0;
    char op = 0;
    const char* p = expr;
    int sign_a = 1, sign_b = 1;
    
    *error = 0;
    
    /* Skip leading whitespace and opening parenthesis */
    while (*p == ' ' || *p == '(' || *p == '\t') p++;
    
    /* Parse first operand with optional sign */
    if (*p == '-') {
        sign_a = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    
    /* Read first number */
    if (*p < '0' || *p > '9') {
        *error = 1;  /* Invalid format - expected number */
        return 0;
    }
    
    while (*p >= '0' && *p <= '9') {
        a = a * 10 + (*p - '0');
        p++;
    }
    a *= sign_a;
    
    /* Skip whitespace before operator */
    while (*p == ' ' || *p == '\t') p++;
    
    /* Get operator */
    op = *p;
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        *error = 3;  /* Unknown operator */
        return 0;
    }
    p++;
    
    /* Skip whitespace after operator */
    while (*p == ' ' || *p == '\t') p++;
    
    /* Parse second operand with optional sign */
    if (*p == '-') {
        sign_b = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    
    /* Read second number */
    if (*p < '0' || *p > '9') {
        *error = 1;  /* Invalid format - expected number */
        return 0;
    }
    
    while (*p >= '0' && *p <= '9') {
        b = b * 10 + (*p - '0');
        p++;
    }
    b *= sign_b;
    
    /* Perform operation */
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0) {
                *error = 2;  /* Division by zero! */
                return 0;
            }
            return a / b;
        default:
            *error = 3;
            return 0;
    }
}

/* ----------------------------------------------------------------------------
 * cmd_help - Show available commands
 * ---------------------------------------------------------------------------- */
void cmd_help(int argc, char* argv[]) {
    vga_print("\n=== SharkOS Commands ===\n\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_print("  ");
        vga_print(commands[i].name);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        /* Pad to align descriptions */
        int pad = 12 - strlen(commands[i].name);
        while (pad-- > 0) vga_putchar(' ');
        
        vga_print("- ");
        vga_print(commands[i].description);
        vga_putchar('\n');
    }
    
    vga_putchar('\n');
}

/* ----------------------------------------------------------------------------
 * cmd_clear - Clear the screen
 * ---------------------------------------------------------------------------- */
void cmd_clear(int argc, char* argv[]) {
    vga_clear();
}

/* ----------------------------------------------------------------------------
 * cmd_echo - Print text to screen
 * ---------------------------------------------------------------------------- */
void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        vga_print(argv[i]);
        if (i < argc - 1) {
            vga_putchar(' ');
        }
    }
    vga_putchar('\n');
}

/* ----------------------------------------------------------------------------
 * cmd_version - Show OS version
 * ---------------------------------------------------------------------------- */
void cmd_version(int argc, char* argv[]) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n  _____ _                _     ___  ____  \n");
    vga_print(" / ____| |              | |   / _ \\/ ___| \n");
    vga_print("| (___ | |__   __ _ _ __| | _| | | \\___ \\ \n");
    vga_print(" \\___ \\| '_ \\ / _` | '__| |/ / | | |___) |\n");
    vga_print(" ____) | | | | (_| | |  |   <| |_| |____/ \n");
    vga_print("|_____/|_| |_|\\__,_|_|  |_|\\_\\\\___/|_____/\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    vga_print("\nVersion: ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print(SHARKOS_VERSION);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    vga_print(" (Codename: ");
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_print(SHARKOS_CODENAME);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print(")\n\n");
    
    vga_print("A simple x86 operating system\n");
    vga_print("Built with NASM + i686-elf-gcc\n\n");
}

/* ----------------------------------------------------------------------------
 * cmd_reboot - Reboot the system
 *
 * Uses the keyboard controller to trigger a reboot.
 * The pulse command (0xFE) tells the keyboard controller to pulse
 * the CPU reset line.
 * ---------------------------------------------------------------------------- */
void cmd_reboot(int argc, char* argv[]) {
    vga_print("Rebooting...\n");
    
    /* Wait for keyboard controller input buffer to be empty */
    while (inb(0x64) & 0x02);
    
    /* Send pulse command: 0xFE pulses the reset line */
    outb(0x64, 0xFE);
    
    /* If that didn't work, halt */
    __asm__ volatile("cli; hlt");
    
    /* Triple fault as last resort (jump to null) */
    ((void(*)(void))0)();
}

/* ----------------------------------------------------------------------------
 * cmd_shutdown - Halt the CPU
 *
 * In a real OS, this would do proper cleanup first.
 * For emulators like QEMU, you can use ACPI for proper shutdown.
 * ---------------------------------------------------------------------------- */
void cmd_shutdown(int argc, char* argv[]) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print("\nSharkOS halted. It is now safe to turn off your computer.\n");
    vga_print("(Press power button or close QEMU window)\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    /* Disable interrupts and halt forever */
    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* ----------------------------------------------------------------------------
 * cmd_calc - Simple calculator
 *
 * Usage: calc (5 + 3)
 *        calc (10 * 2)
 *        calc (100 / 5)
 *        calc (50 - 25)
 *
 * Supports: + - * /
 * Detects: Division by zero, invalid format
 * ---------------------------------------------------------------------------- */
void cmd_calc(int argc, char* argv[]) {
    char expr[128];
    int error = 0;
    int result;
    int i;
    
    if (argc < 2) {
        vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
        vga_print("Usage: calc (operand1 operator operand2)\n");
        vga_print("Example: calc (5 + 3)\n");
        vga_print("         calc (10 * 2)\n");
        vga_print("         calc (100 / 5)\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }
    
    /* Reconstruct the expression from argv */
    expr[0] = '\0';
    for (i = 1; i < argc; i++) {
        strcat(expr, argv[i]);
        if (i < argc - 1) strcat(expr, " ");
    }
    
    /* Evaluate the expression */
    result = calc_evaluate(expr, &error);
    
    /* Handle errors with helpful messages */
    switch (error) {
        case 0:
            /* Success - print result */
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_print("= ");
            vga_print_int(result);
            vga_putchar('\n');
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            break;
            
        case 1:
            /* Invalid format */
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print("Error: Invalid expression format\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            vga_print("Expected: (number operator number)\n");
            break;
            
        case 2:
            /* Division by zero */
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print("Error: Division by zero!\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            vga_print("Cannot divide by zero. Mathematics forbids it.\n");
            break;
            
        case 3:
            /* Unknown operator */
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print("Error: Unknown operator\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            vga_print("Supported operators: + - * /\n");
            break;
            
        default:
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print("Error: Unknown error occurred\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            break;
    }
}

/* ----------------------------------------------------------------------------
 * cmd_colors - Show VGA colors
 *
 * Usage: colors [1-15]
 * Without argument, shows all 16 colors.
 * With argument, shows text in that color.
 * ---------------------------------------------------------------------------- */
void cmd_colors(int argc, char* argv[]) {
    static const char* color_names[] = {
        "BLACK",        "BLUE",         "GREEN",        "CYAN",
        "RED",          "MAGENTA",      "BROWN",        "LIGHT_GREY",
        "DARK_GREY",    "LIGHT_BLUE",   "LIGHT_GREEN",  "LIGHT_CYAN",
        "LIGHT_RED",    "LIGHT_MAGENTA","YELLOW",       "WHITE"
    };
    
    if (argc >= 2) {
        /* Show specific color */
        int color = atoi(argv[1]);
        if (color < 0 || color > 15) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print("Error: Color must be 0-15\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            return;
        }
        
        vga_print("Color ");
        vga_print_int(color);
        vga_print(": ");
        vga_set_color((vga_color_t)color, VGA_COLOR_BLACK);
        vga_print(color_names[color]);
        vga_print(" - The quick brown fox jumps over the lazy dog");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_putchar('\n');
        return;
    }
    
    /* Show all colors */
    vga_print("\n=== VGA Color Palette ===\n\n");
    
    for (int i = 0; i < 16; i++) {
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        /* Print color number with padding */
        if (i < 10) vga_putchar(' ');
        vga_print_int(i);
        vga_print(": ");
        
        /* Print color name in that color */
        vga_set_color((vga_color_t)i, VGA_COLOR_BLACK);
        
        /* Special case: black on black is invisible, use white background */
        if (i == 0) {
            vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
        }
        
        vga_print(color_names[i]);
        
        /* Reset and add sample text */
        vga_set_color((vga_color_t)i, VGA_COLOR_BLACK);
        if (i == 0) {
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
        vga_print(" - Sample Text\n");
    }
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_putchar('\n');
}

/* ----------------------------------------------------------------------------
 * shell_process_command - Process a command line
 * ---------------------------------------------------------------------------- */
void shell_process_command(const char* line) {
    char buffer[SHELL_MAX_INPUT];
    char* argv[SHELL_MAX_ARGS];
    int argc;
    int i;
    
    /* Skip empty lines */
    if (line[0] == '\0') {
        return;
    }
    
    /* Copy line to buffer (we'll modify it during parsing) */
    strncpy(buffer, line, SHELL_MAX_INPUT - 1);
    buffer[SHELL_MAX_INPUT - 1] = '\0';
    
    /* Parse into argc/argv */
    argc = parse_arguments(buffer, argv, SHELL_MAX_ARGS);
    
    if (argc == 0) {
        return;
    }
    
    /* Look up command in table */
    for (i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            /* Found it - execute! */
            commands[i].func(argc, argv);
            return;
        }
    }
    
    /* Command not found */
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print("Unknown command: ");
    vga_print(argv[0]);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("\nType 'help' for available commands.\n");
}

/* ----------------------------------------------------------------------------
 * shell_run - Main shell loop (never returns)
 * ---------------------------------------------------------------------------- */
void shell_run(void) {
    char input[SHELL_MAX_INPUT];
    
    /* Welcome message */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print("\n*** Welcome to SharkOS ***\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("Type 'help' for available commands.\n\n");
    
    /* Main loop */
    while (1) {
        /* Print prompt */
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print(SHELL_PROMPT);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        /* Read command */
        keyboard_readline(input, SHELL_MAX_INPUT);
        
        /* Process command */
        shell_process_command(input);
    }
}
