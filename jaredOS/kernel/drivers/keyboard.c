/**
 * jaredOS - Keyboard Driver Implementation
 */

#include "keyboard.h"
#include "../core/irq.h"
#include "../types.h"

/* Keyboard ports */
#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64

/* Key buffer */
#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static volatile int buffer_start = 0;
static volatile int buffer_end = 0;

/* Modifier state */
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool caps_lock = false;

/* US keyboard scancode to ASCII (lowercase) */
static const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

/* US keyboard scancode to ASCII (uppercase/shifted) */
static const char scancode_to_ascii_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

/**
 * Add key to buffer
 */
static void buffer_add(char c) {
    int next = (buffer_end + 1) % KEY_BUFFER_SIZE;
    if (next != buffer_start) {
        key_buffer[buffer_end] = c;
        buffer_end = next;
    }
}

/**
 * Keyboard interrupt handler
 */
static void keyboard_handler(registers_t *regs) {
    (void)regs;
    
    uint8_t scancode = inb(KB_DATA_PORT);

    /* Handle key release (high bit set) */
    if (scancode & 0x80) {
        scancode &= 0x7F;
        /* Left/Right Shift release */
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = false;
        }
        /* Left/Right Ctrl release */
        if (scancode == 0x1D) {
            ctrl_pressed = false;
        }
        return;
    }

    /* Handle modifier keys (press) */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }

    if (scancode == 0x1D) {  /* Left Ctrl */
        ctrl_pressed = true;
        return;
    }

    if (scancode == 0x3A) {  /* Caps Lock */
        caps_lock = !caps_lock;
        return;
    }

    /* Convert scancode to ASCII */
    if (scancode < sizeof(scancode_to_ascii)) {
        char c;
        bool use_upper = shift_pressed;
        
        /* Handle caps lock for letters */
        if (scancode >= 0x10 && scancode <= 0x19) use_upper ^= caps_lock;  /* Top row */
        if (scancode >= 0x1E && scancode <= 0x26) use_upper ^= caps_lock;  /* Middle row */
        if (scancode >= 0x2C && scancode <= 0x32) use_upper ^= caps_lock;  /* Bottom row */

        if (use_upper) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        /* Handle Ctrl + letter (generate control codes 1-26) */
        if (ctrl_pressed && c >= 'a' && c <= 'z') {
            c = c - 'a' + 1;  /* Ctrl+A=1, Ctrl+B=2, ..., Ctrl+Z=26 */
        } else if (ctrl_pressed && c >= 'A' && c <= 'Z') {
            c = c - 'A' + 1;
        }

        if (c != 0) {
            buffer_add(c);
        }
    }
}

/**
 * Initialize keyboard
 */
void keyboard_init(void) {
    buffer_start = 0;
    buffer_end = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    caps_lock = false;
    irq_register_handler(1, keyboard_handler);
}

/**
 * Check if a key is available
 */
bool keyboard_has_key(void) {
    return buffer_start != buffer_end;
}

/**
 * Get character (blocking)
 */
char keyboard_getchar(void) {
    while (buffer_start == buffer_end) {
        __asm__ volatile ("hlt");
    }
    char c = key_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEY_BUFFER_SIZE;
    return c;
}

/**
 * Get last key without blocking
 */
char keyboard_get_last_key(void) {
    if (buffer_start == buffer_end) {
        return 0;
    }
    char c = key_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEY_BUFFER_SIZE;
    return c;
}
