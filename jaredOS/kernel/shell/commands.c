/**
 * jaredOS - Commands Implementation
 */

#include "commands.h"
#include "calc.h"
#include "editor.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../drivers/ata.h"
#include "../memory/pmm.h"
#include "../fs/simplefs.h"
#include "../../lang/gwango/core/gwango.h"
#include "../lib/printf.h"
#include "../lib/string.h"
#include "../lib/stdlib.h"

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
    {"mem",    "Show memory usage",        cmd_mem},
    {"dump",   "Hex dump memory",          cmd_dump},
    {"edit",   "Text editor",              cmd_edit},
    {"gwan",   "Gwango interpreter",       cmd_gwan},
    {"ls",     "List files",               cmd_ls},
    {"cat",    "Print file contents",      cmd_cat},
    {"write",  "Write text to file",       cmd_write},
    {"format", "Format disk",              cmd_format},
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

/**
 * Memory info command
 */
void cmd_mem(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    uint32_t total = pmm_get_total_memory();
    uint32_t used = pmm_get_used_memory();
    uint32_t free_mem = total - used;
    
    kprintf("\nMemory Information:\n");
    kprintf("-------------------\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("  Total:  ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("%u KB\n", total);
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    kprintf("  Used:   ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("%u KB\n", used);
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("  Free:   ");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    kprintf("%u KB\n\n", free_mem);
}

/**
 * Hex dump command
 * Usage: dump <address> [length]
 */
void cmd_dump(int argc, char *argv[]) {
    if (argc < 2) {
        kprintf("Usage: dump <address> [length]\n");
        kprintf("Example: dump 0x100000 64\n");
        return;
    }
    
    /* Parse address (handle 0x prefix) */
    const char *addr_str = argv[1];
    uint32_t addr = 0;
    
    if (addr_str[0] == '0' && (addr_str[1] == 'x' || addr_str[1] == 'X')) {
        addr_str += 2;
    }
    
    /* Convert hex string to number */
    while (*addr_str) {
        addr <<= 4;
        if (*addr_str >= '0' && *addr_str <= '9') {
            addr |= *addr_str - '0';
        } else if (*addr_str >= 'a' && *addr_str <= 'f') {
            addr |= *addr_str - 'a' + 10;
        } else if (*addr_str >= 'A' && *addr_str <= 'F') {
            addr |= *addr_str - 'A' + 10;
        }
        addr_str++;
    }
    
    /* Parse length (default 64 bytes) */
    uint32_t len = 64;
    if (argc >= 3) {
        len = (uint32_t)atoi(argv[2]);
    }
    if (len > 256) len = 256;  /* Limit for display */
    
    uint8_t *ptr = (uint8_t*)addr;
    
    kprintf("\nDump of 0x%x (%u bytes):\n", addr, len);
    
    for (uint32_t i = 0; i < len; i += 16) {
        /* Print address */
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        kprintf("%x: ", addr + i);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        /* Print hex bytes */
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t b = ptr[i + j];
            if (b < 16) kprintf("0");
            kprintf("%x ", b);
        }
        
        /* Padding if less than 16 bytes */
        for (uint32_t j = len - i; j < 16 && i + 16 > len; j++) {
            kprintf("   ");
        }
        
        /* Print ASCII */
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf(" |");
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t b = ptr[i + j];
            if (b >= 32 && b < 127) {
                vga_putchar(b);
            } else {
                vga_putchar('.');
            }
        }
        kprintf("|\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    kprintf("\n");
}

/**
 * Edit command - open text editor
 */
void cmd_edit(int argc, char *argv[]) {
    const char *filename = NULL;
    if (argc >= 2) {
        filename = argv[1];
    }
    editor_open(filename);
}

/**
 * List files command
 */
void cmd_ls(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    if (!fs_ready()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Filesystem not ready. Use 'format' first.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    fs_file_t files[FS_MAX_FILES];
    int count = fs_list(files, FS_MAX_FILES);
    
    if (count == 0) {
        kprintf("No files found.\n");
        return;
    }
    
    kprintf("\nFiles:\n");
    kprintf("------\n");
    for (int i = 0; i < count; i++) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        kprintf("  %s", files[i].name);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        /* Pad name */
        int len = strlen(files[i].name);
        for (int j = len; j < 20; j++) vga_putchar(' ');
        kprintf("%u bytes\n", files[i].size);
    }
    kprintf("\n%d file(s)\n\n", count);
}

/**
 * Cat command - print file contents
 */
void cmd_cat(int argc, char *argv[]) {
    if (argc < 2) {
        kprintf("Usage: cat <filename>\n");
        return;
    }
    
    if (!fs_ready()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Filesystem not ready.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    static char buffer[4096];
    int bytes = fs_read(argv[1], buffer, sizeof(buffer) - 1);
    
    if (bytes < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("File not found: %s\n", argv[1]);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    buffer[bytes] = '\0';
    kprintf("%s", buffer);
    if (bytes > 0 && buffer[bytes - 1] != '\n') {
        kprintf("\n");
    }
}

/**
 * Write command - write text to file
 */
void cmd_write(int argc, char *argv[]) {
    if (argc < 3) {
        kprintf("Usage: write <filename> <text...>\n");
        kprintf("Example: write hello.txt Hello World!\n");
        return;
    }
    
    if (!fs_ready()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Filesystem not ready. Use 'format' first.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    /* Concatenate all remaining args */
    static char buffer[1024];
    buffer[0] = '\0';
    
    for (int i = 2; i < argc; i++) {
        strcat(buffer, argv[i]);
        if (i < argc - 1) strcat(buffer, " ");
    }
    strcat(buffer, "\n");
    
    if (fs_write(argv[1], buffer, strlen(buffer))) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("Written %u bytes to %s\n", strlen(buffer), argv[1]);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Failed to write file.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/**
 * Format command - format disk
 */
void cmd_format(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    if (!ata_drive_present()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("No disk detected.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return;
    }
    
    kprintf("Formatting disk...\n");
    
    if (fs_format()) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("Disk formatted successfully!\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Format failed.\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

/**
 * Gwango language interpreter
 * Usage: gwan [-d] [file.gw]
 *   -d  Dump generated x86 bytecode
 */
void cmd_gwan(int argc, char *argv[]) {
    if (argc < 2) {
        /* REPL mode */
        gwango_repl();
    } else if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        /* Dump mode REPL - show example */
        kprintf("Usage: gwan -d <code>\n");
        kprintf("Example: gwan -d \"var x = 5\"\n");
    } else if (argc >= 3 && strcmp(argv[1], "-d") == 0) {
        /* Dump mode - check if file exists first */
        fs_file_t info;
        if (fs_stat(argv[2], &info)) {
            gwango_dump_file(argv[2]);
        } else {
            /* Treat as inline code */
            gwango_dump(argv[2]);
        }
    } else {
        /* Run file */
        gwango_run_file(argv[1]);
    }
}
