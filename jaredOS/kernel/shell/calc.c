/**
 * jaredOS - Calculator Implementation
 */

#include "calc.h"
#include "../lib/printf.h"
#include "../lib/stdlib.h"
#include "../lib/string.h"
#include "../drivers/vga.h"

/**
 * Execute calculator command
 * Usage: calc <num1> <op> <num2>
 * Example: calc 10 + 5
 */
void calc_execute(int argc, char *argv[]) {
    if (argc < 4) {
        kprintf("Usage: calc <num1> <op> <num2>\n");
        kprintf("Example: calc 10 + 5\n");
        kprintf("Operators: + - * /\n");
        return;
    }
    
    int num1 = atoi(argv[1]);
    char op = argv[2][0];
    int num2 = atoi(argv[3]);
    int result = 0;
    bool valid = true;
    
    switch (op) {
        case '+':
            result = num1 + num2;
            break;
        case '-':
            result = num1 - num2;
            break;
        case '*':
            result = num1 * num2;
            break;
        case '/':
            if (num2 == 0) {
                vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                kprintf("Error: Division by zero!\n");
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                return;
            }
            result = num1 / num2;
            break;
        default:
            valid = false;
            break;
    }
    
    if (valid) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        kprintf("%d %c %d = %d\n", num1, op, num2, result);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        kprintf("Unknown operator: %c\n", op);
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        kprintf("Valid operators: + - * /\n");
    }
}
