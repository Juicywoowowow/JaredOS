/**
 * jaredOS - Printf Implementation
 */

#include "printf.h"
#include "stdlib.h"
#include "string.h"
#include "../drivers/vga.h"

/* Variable arguments */
typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)

/**
 * Print to buffer (base function)
 */
static int vsprintf(char *buffer, const char *format, va_list args) {
    char *buf = buffer;
    char temp[32];
    
    while (*format) {
        if (*format != '%') {
            *buf++ = *format++;
            continue;
        }
        
        format++;  /* Skip '%' */
        
        switch (*format) {
            case 'd':
            case 'i': {
                int val = va_arg(args, int);
                itoa(val, temp, 10);
                char *t = temp;
                while (*t) *buf++ = *t++;
                break;
            }
            
            case 'u': {
                uint32_t val = va_arg(args, uint32_t);
                utoa(val, temp, 10);
                char *t = temp;
                while (*t) *buf++ = *t++;
                break;
            }
            
            case 'x':
            case 'X': {
                uint32_t val = va_arg(args, uint32_t);
                utoa(val, temp, 16);
                char *t = temp;
                while (*t) *buf++ = *t++;
                break;
            }
            
            case 'c': {
                char c = (char)va_arg(args, int);
                *buf++ = c;
                break;
            }
            
            case 's': {
                char *s = va_arg(args, char*);
                if (s == NULL) s = "(null)";
                while (*s) *buf++ = *s++;
                break;
            }
            
            case 'p': {
                uint32_t val = (uint32_t)va_arg(args, void*);
                *buf++ = '0';
                *buf++ = 'x';
                utoa(val, temp, 16);
                char *t = temp;
                while (*t) *buf++ = *t++;
                break;
            }
            
            case '%':
                *buf++ = '%';
                break;
            
            default:
                *buf++ = '%';
                *buf++ = *format;
                break;
        }
        
        format++;
    }
    
    *buf = '\0';
    return (int)(buf - buffer);
}

/**
 * Kernel printf
 */
int kprintf(const char *format, ...) {
    char buffer[1024];
    va_list args;
    
    va_start(args, format);
    int len = vsprintf(buffer, format, args);
    va_end(args);
    
    vga_puts(buffer);
    return len;
}

/**
 * Sprintf to buffer
 */
int ksprintf(char *buffer, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int len = vsprintf(buffer, format, args);
    va_end(args);
    return len;
}
