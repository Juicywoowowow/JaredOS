/**
 * Gwango Language - Main Runtime
 */

#include "gwango.h"
#include "lexer.h"
#include "parser.h"
#include "jit.h"
#include "../../../kernel/drivers/vga.h"
#include "../../../kernel/drivers/keyboard.h"
#include "../../../kernel/fs/simplefs.h"
#include "../../../kernel/lib/printf.h"
#include "../../../kernel/lib/string.h"

/* Code buffer for JIT - needs to be executable */
static uint8_t __attribute__((aligned(4096))) jit_code[JIT_CODE_SIZE];

/* Run source code */
bool gwango_run(const char *source) {
    parser_t parser;
    jit_t jit;
    
    /* Parse */
    parser_init(&parser, source);
    ast_node_t *program = parser_parse(&parser);
    
    if (parser.had_error) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Parse error: %s\n", parser.error_msg);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return false;
    }
    
    /* Compile to x86 */
    jit_init(&jit, jit_code);
    bool ok = jit_compile(&jit, program);
    
    if (!ok) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Compile error: %s\n", jit.error_msg);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        ast_free(program);
        return false;
    }
    
    /* Execute JIT code! */
    jit_func_t entry = jit_get_entry(&jit);
    if (entry) {
        entry();
    }
    
    ast_free(program);
    return true;
}

/* Dump generated x86 bytecode */
bool gwango_dump(const char *source) {
    parser_t parser;
    jit_t jit;
    
    parser_init(&parser, source);
    ast_node_t *program = parser_parse(&parser);
    
    if (parser.had_error) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Parse error: %s\n", parser.error_msg);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return false;
    }
    
    jit_init(&jit, jit_code);
    bool ok = jit_compile(&jit, program);
    
    if (!ok) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Compile error: %s\n", jit.error_msg);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        ast_free(program);
        return false;
    }
    
    /* Dump disassembly */
    jit_disassemble(&jit);
    
    ast_free(program);
    return true;
}

/* Dump generated x86 code from file */
bool gwango_dump_file(const char *filename) {
    if (!fs_ready()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Filesystem not ready\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return false;
    }
    
    static char file_buf[4096];
    int bytes = fs_read(filename, file_buf, sizeof(file_buf) - 1);
    
    if (bytes < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("File not found: %s\n", filename);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return false;
    }
    
    file_buf[bytes] = '\0';
    return gwango_dump(file_buf);
}

/* Run file */
bool gwango_run_file(const char *filename) {
    if (!fs_ready()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Filesystem not ready\n");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return false;
    }
    
    static char file_buf[4096];
    int bytes = fs_read(filename, file_buf, sizeof(file_buf) - 1);
    
    if (bytes < 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("File not found: %s\n", filename);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return false;
    }
    
    file_buf[bytes] = '\0';
    return gwango_run(file_buf);
}

/* REPL mode */
void gwango_repl(void) {
    static char line[256];
    int pos = 0;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("Gwango REPL v0.1\n");
    kprintf("Type 'exit' to quit\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    while (1) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("> ");
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        /* Read line */
        pos = 0;
        while (pos < 255) {
            char c = keyboard_getchar();
            
            if (c == '\n') {
                kprintf("\n");
                break;
            } else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    kprintf("\b \b");
                }
            } else if (c >= 32 && c < 127) {
                line[pos++] = c;
                vga_putchar(c);
            }
        }
        line[pos] = '\0';
        
        /* Check for exit */
        if (strcmp(line, "exit") == 0) {
            break;
        }
        
        /* Run line */
        if (pos > 0) {
            gwango_run(line);
        }
    }
    
    kprintf("Goodbye!\n");
}
