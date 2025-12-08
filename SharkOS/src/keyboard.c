/*
 * =============================================================================
 * SharkOS Keyboard Driver (keyboard.c)
 * =============================================================================
 * PS/2 keyboard driver implementation.
 *
 * The keyboard sends scancodes when keys are pressed/released.
 * We convert these to ASCII characters for the shell.
 *
 * DEBUGGING TIPS:
 *   - Port 0x60 = keyboard data port (read scancodes here)
 *   - Port 0x64 = keyboard status/command port
 *   - Bit 0 of status port = 1 if data is available
 *   - If keys don't work, check if you're reading make codes vs break codes
 *   - Scancodes differ from ASCII! Use a conversion table.
 * =============================================================================
 */

#include "keyboard.h"
#include "io.h"
#include "vga.h"

/* ----------------------------------------------------------------------------
 * Keyboard I/O Ports
 * ---------------------------------------------------------------------------- */
#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Status port bits */
#define KEYBOARD_STATUS_OUTPUT_FULL 0x01

/* ----------------------------------------------------------------------------
 * Scancode to ASCII Lookup Table (US Layout, Set 1)
 *
 * This is for "make" codes (key press). Index = scancode, value = ASCII.
 * 0 means no printable character for that scancode.
 * 
 * NOTE: This is a simplified table. Real keyboard drivers handle:
 *   - Shift/Ctrl/Alt modifiers
 *   - Extended scancodes (0xE0 prefix)
 *   - Caps lock, num lock
 *   - Key repeat
 * ---------------------------------------------------------------------------- */
static const char scancode_to_ascii[] = {
    0,    0,    '1',  '2',  '3',  '4',  '5',  '6',   /* 0x00 - 0x07 */
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',  /* 0x08 - 0x0F */
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',   /* 0x10 - 0x17 */
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',   /* 0x18 - 0x1F */
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',   /* 0x20 - 0x27 */
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',   /* 0x28 - 0x2F */
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',   /* 0x30 - 0x37 */
    0,    ' ',  0,    0,    0,    0,    0,    0,     /* 0x38 - 0x3F */
    0,    0,    0,    0,    0,    0,    0,    '7',   /* 0x40 - 0x47 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',   /* 0x48 - 0x4F */
    '2',  '3',  '0',  '.',  0,    0,    0,    0      /* 0x50 - 0x57 */
};

/* Shifted version of the scancode table */
static const char scancode_to_ascii_shift[] = {
    0,    0,    '!',  '@',  '#',  '$',  '%',  '^',   /* 0x00 - 0x07 */
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',  /* 0x08 - 0x0F */
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   /* 0x10 - 0x17 */
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',   /* 0x18 - 0x1F */
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',   /* 0x20 - 0x27 */
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',   /* 0x28 - 0x2F */
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',   /* 0x30 - 0x37 */
    0,    ' ',  0,    0,    0,    0,    0,    0,     /* 0x38 - 0x3F */
    0,    0,    0,    0,    0,    0,    0,    '7',   /* 0x40 - 0x47 */
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',   /* 0x48 - 0x4F */
    '2',  '3',  '0',  '.',  0,    0,    0,    0      /* 0x50 - 0x57 */
};

#define SCANCODE_TABLE_SIZE (sizeof(scancode_to_ascii) / sizeof(scancode_to_ascii[0]))

/* Special scancodes */
#define SCANCODE_LEFT_SHIFT_PRESS   0x2A
#define SCANCODE_LEFT_SHIFT_RELEASE 0xAA
#define SCANCODE_RIGHT_SHIFT_PRESS  0x36
#define SCANCODE_RIGHT_SHIFT_RELEASE 0xB6
#define SCANCODE_LEFT_CTRL_PRESS    0x1D
#define SCANCODE_LEFT_CTRL_RELEASE  0x9D

/* Keyboard state */
static bool shift_pressed = false;
static bool ctrl_pressed = false;

/* ----------------------------------------------------------------------------
 * keyboard_get_state - Get current keyboard state
 * ---------------------------------------------------------------------------- */
keyboard_state_t keyboard_get_state(void) {
    keyboard_state_t state;
    state.shift_pressed = shift_pressed;
    state.ctrl_pressed = ctrl_pressed;
    state.alt_pressed = false; /* Not implemented yet */
    return state;
}

/* ----------------------------------------------------------------------------
 * keyboard_init - Initialize keyboard driver
 * ---------------------------------------------------------------------------- */
void keyboard_init(void) {
    /* Flush the keyboard buffer */
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL) {
        inb(KEYBOARD_DATA_PORT);
    }
    
    shift_pressed = false;
    ctrl_pressed = false;
}

/* ----------------------------------------------------------------------------
 * keyboard_has_key - Check if a key is available
 * ---------------------------------------------------------------------------- */
bool keyboard_has_key(void) {
    return (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL) != 0;
}

/* ----------------------------------------------------------------------------
 * keyboard_getchar - Read a single character (blocking)
 * ---------------------------------------------------------------------------- */
char keyboard_getchar(void) {
    uint8_t scancode;
    char c;
    
    while (1) {
        /* Wait for key data to be available */
        while (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL)) {
            /* Busy wait */
        }
        
        scancode = inb(KEYBOARD_DATA_PORT);
        
        /* Handle shift key state */
        if (scancode == SCANCODE_LEFT_SHIFT_PRESS || 
            scancode == SCANCODE_RIGHT_SHIFT_PRESS) {
            shift_pressed = true;
            continue;
        }
        
        if (scancode == SCANCODE_LEFT_SHIFT_RELEASE || 
            scancode == SCANCODE_RIGHT_SHIFT_RELEASE) {
            shift_pressed = false;
            continue;
        }

        /* Handle Control key state */
        if (scancode == SCANCODE_LEFT_CTRL_PRESS) {
            ctrl_pressed = true;
            continue;
        }

        if (scancode == SCANCODE_LEFT_CTRL_RELEASE) {
            ctrl_pressed = false;
            continue;
        }
        
        /* Ignore key release events */
        if (scancode & 0x80) {
            continue;
        }
        
        /* Convert scancode to ASCII */
        if (scancode < SCANCODE_TABLE_SIZE) {
            if (shift_pressed) {
                c = scancode_to_ascii_shift[scancode];
            } else {
                c = scancode_to_ascii[scancode];
            }
            
            /* Apply Control modifier */
            if (ctrl_pressed) {
                /* Ctrl+A (1) to Ctrl+Z (26) */
                if (c >= 'a' && c <= 'z') {
                    return c - 'a' + 1;
                }
                if (c >= 'A' && c <= 'Z') {
                    return c - 'A' + 1;
                }
            }
            
            if (c != 0) {
                return c;
            }
        }
    }
}

/* ----------------------------------------------------------------------------
 * keyboard_readline - Read a line of input
 *
 * Reads characters until Enter is pressed or buffer is full.
 * Handles backspace for editing.
 * Echoes characters to screen.
 *
 * @param buffer: Buffer to store the line (will be null-terminated)
 * @param size: Size of buffer (including null terminator)
 * ---------------------------------------------------------------------------- */
void keyboard_readline(char* buffer, size_t size) {
    size_t pos = 0;
    char c;
    
    if (size == 0) return;
    
    while (1) {
        c = keyboard_getchar();
        
        if (c == '\n') {
            /* Enter pressed - finish input */
            buffer[pos] = '\0';
            vga_putchar('\n');
            return;
        }
        
        if (c == '\b') {
            /* Backspace - remove last character */
            if (pos > 0) {
                pos--;
                vga_putchar('\b');  /* This erases the character on screen */
            }
            continue;
        }
        
        /* Regular character - add to buffer if space available */
        if (pos < size - 1) {
            buffer[pos++] = c;
            vga_putchar(c);  /* Echo to screen */
        }
    }
}
