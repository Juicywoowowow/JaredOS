/**
 * jaredOS - Keyboard Driver Header
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../types.h"

/* Special keys */
#define KEY_ENTER       '\n'
#define KEY_BACKSPACE   '\b'
#define KEY_TAB         '\t'
#define KEY_ESCAPE      27

/* Initialize keyboard driver */
void keyboard_init(void);

/* Get character (blocking) */
char keyboard_getchar(void);

/* Check if a key is available */
bool keyboard_has_key(void);

/* Get last key without blocking */
char keyboard_get_last_key(void);

#endif /* KEYBOARD_H */
