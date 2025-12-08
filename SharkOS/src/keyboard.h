/*
 * =============================================================================
 * SharkOS Keyboard Driver Header (keyboard.h)
 * =============================================================================
 * PS/2 keyboard driver interface.
 * =============================================================================
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * Keyboard Functions
 * ---------------------------------------------------------------------------- */

/* Initialize keyboard driver */
void keyboard_init(void);

/* Read a single character (blocking) */
char keyboard_getchar(void);

/* Read a line of input into buffer (blocking) */
/* Returns when Enter is pressed. Max length is (size - 1) for null terminator. */
void keyboard_readline(char* buffer, size_t size);

/* Check if a key is available (non-blocking) */
bool keyboard_has_key(void);

/* Keyboard state structure */
typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
} keyboard_state_t;

/* Get current keyboard state */
keyboard_state_t keyboard_get_state(void);

#endif /* KEYBOARD_H */
