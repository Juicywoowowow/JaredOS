/*
 * =============================================================================
 * keyboard.c - PS/2 Keyboard Driver for FoxOS
 * =============================================================================
 *
 * This driver handles the PS/2 keyboard controller. It:
 *   1. Receives scancodes when keys are pressed/released
 *   2. Converts scancodes to ASCII characters
 *   3. Maintains a keyboard input buffer
 *   4. Tracks modifier key states (Shift, Ctrl, Alt)
 *
 * DEBUGGING TIPS:
 *   - If no keys work, check that IRQ1 is properly set up
 *   - If keys give wrong characters, the scancode table may be wrong
 *   - Use debug_print to output raw scancodes for debugging
 *   - The keyboard controller uses ports 0x60 (data) and 0x64 (command)
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: Keyboard Controller Ports and Constants
 * =============================================================================
 */

#define KB_DATA_PORT 0x60   /* Keyboard data port */
#define KB_STATUS_PORT 0x64 /* Keyboard status port */
#define KB_CMD_PORT 0x64    /* Keyboard command port */

/* Status register bits */
#define KB_STATUS_OUTPUT_FULL 0x01 /* Output buffer has data */
#define KB_STATUS_INPUT_FULL 0x02  /* Input buffer has data */

/* Special scancodes */
#define SC_ESCAPE 0x01
#define SC_BACKSPACE 0x0E
#define SC_TAB 0x0F
#define SC_ENTER 0x1C
#define SC_LCTRL 0x1D
#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_LALT 0x38
#define SC_CAPSLOCK 0x3A
#define SC_F1 0x3B
#define SC_F12 0x58

/* Key release flag (bit 7 set in scancode) */
#define SC_RELEASE 0x80

/* =============================================================================
 * SECTION 2: Keyboard State
 * =============================================================================
 */

/* Modifier key states */
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock = false;

/* Input buffer (circular buffer) */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static uint32_t kb_buffer_head = 0; /* Write position */
static uint32_t kb_buffer_tail = 0; /* Read position */

/* Last pressed key (for immediate access) */
static char last_key = 0;

/* =============================================================================
 * SECTION 3: Scancode to ASCII Conversion Tables
 * =============================================================================
 * US QWERTY keyboard layout
 */

/* Normal (lowercase) characters */
static const char
    scancode_to_ascii[128] =
        {
            0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
            '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
            'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
            'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
            'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0,   0,
            0,   0,   0,    0,    0,    0,   0,   0,    0, /* F1-F10 */
            0,   0, /* Num Lock, Scroll Lock */
            0,   0,   0,    '-',  0,    0,   0,   '+',  0,   0,   0,   0,
            0,           /* Numpad */
            0,   0,      /* more keys... */
            0,   0,   0, /* F11, F12 */
};

/* Shifted (uppercase) characters */
static const char
    scancode_to_ascii_shift[128] =
        {
            0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
            '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
            'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
            'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
            'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0,   0,
            0,   0,   0,    0,    0,    0,   0,   0,   0, /* F1-F10 */
            0,   0, /* Num Lock, Scroll Lock */
            0,   0,   0,    '-',  0,    0,   0,   '+', 0,   0,   0,   0,
            0,                       /* Numpad */
            0,   0,   0,    0,    0, /* F11, F12 */
};

/* =============================================================================
 * SECTION 4: Buffer Operations
 * =============================================================================
 */

/*
 * kb_buffer_put - Add a character to the keyboard buffer
 *
 * @param c: Character to add
 *
 * Overwrites oldest data if buffer is full.
 */
static void kb_buffer_put(char c) {
  uint32_t next_head = (kb_buffer_head + 1) % KB_BUFFER_SIZE;

  /* Check for buffer overflow */
  if (next_head == kb_buffer_tail) {
    /* Buffer full - drop oldest character */
    kb_buffer_tail = (kb_buffer_tail + 1) % KB_BUFFER_SIZE;
  }

  kb_buffer[kb_buffer_head] = c;
  kb_buffer_head = next_head;
}

/*
 * kb_buffer_get - Get a character from the keyboard buffer
 *
 * Returns: Next character in buffer, or 0 if buffer is empty
 */
char kb_buffer_get(void) {
  if (kb_buffer_head == kb_buffer_tail) {
    return 0; /* Buffer empty */
  }

  char c = kb_buffer[kb_buffer_tail];
  kb_buffer_tail = (kb_buffer_tail + 1) % KB_BUFFER_SIZE;
  return c;
}

/*
 * kb_buffer_available - Check if characters are available
 *
 * Returns: Number of characters in buffer
 */
uint32_t kb_buffer_available(void) {
  if (kb_buffer_head >= kb_buffer_tail) {
    return kb_buffer_head - kb_buffer_tail;
  } else {
    return KB_BUFFER_SIZE - kb_buffer_tail + kb_buffer_head;
  }
}

/* =============================================================================
 * SECTION 5: Keyboard Initialization
 * =============================================================================
 */

/*
 * keyboard_init - Initialize the PS/2 keyboard
 *
 * Clears any pending data and resets keyboard state.
 */
void keyboard_init(void) {
  /* Clear any pending data in the keyboard buffer */
  while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) {
    inb(KB_DATA_PORT);
  }

  /* Reset state */
  shift_pressed = false;
  ctrl_pressed = false;
  alt_pressed = false;
  caps_lock = false;

  kb_buffer_head = 0;
  kb_buffer_tail = 0;
  last_key = 0;

  debug_print("[KB] Keyboard initialized (US QWERTY layout)\n");
}

/* =============================================================================
 * SECTION 6: Keyboard Interrupt Handler
 * =============================================================================
 */

/*
 * keyboard_handler - Called on keyboard interrupt (IRQ1)
 *
 * Reads the scancode and converts it to ASCII.
 * Called from irq_handler in kernelA.c.
 */
void keyboard_handler(void) {
  uint8_t scancode = inb(KB_DATA_PORT);

/* Debug: print raw scancode */
#if 0
    debug_print("[KB] Scancode: ");
    debug_hex(scancode);
    debug_print("\n");
#endif

  /* Check if this is a key release */
  if (scancode & SC_RELEASE) {
    uint8_t key = scancode & ~SC_RELEASE;

    /* Update modifier states on release */
    switch (key) {
    case SC_LSHIFT:
    case SC_RSHIFT:
      shift_pressed = false;
      break;
    case SC_LCTRL:
      ctrl_pressed = false;
      break;
    case SC_LALT:
      alt_pressed = false;
      break;
    }

    return; /* Don't process release events further */
  }

  /* Handle key press */
  switch (scancode) {
  case SC_LSHIFT:
  case SC_RSHIFT:
    shift_pressed = true;
    return;

  case SC_LCTRL:
    ctrl_pressed = true;
    return;

  case SC_LALT:
    alt_pressed = true;
    return;

  case SC_CAPSLOCK:
    caps_lock = !caps_lock;
    debug_print("[KB] Caps Lock: ");
    debug_print(caps_lock ? "ON\n" : "OFF\n");
    return;

  case SC_ESCAPE:
    /* Could trigger a menu or cancel operation */
    debug_print("[KB] Escape pressed\n");
    return;
  }

  /* Convert scancode to ASCII */
  char c;
  bool use_shift = shift_pressed;

  /* Caps lock affects only letters */
  if (caps_lock && scancode >= 0x10 && scancode <= 0x32) {
    use_shift = !use_shift;
  }

  if (use_shift) {
    c = scancode_to_ascii_shift[scancode];
  } else {
    c = scancode_to_ascii[scancode];
  }

  /* Only buffer valid characters */
  if (c != 0) {
    last_key = c;
    kb_buffer_put(c);

/* Debug output */
#if 0
        debug_print("[KB] Key: '");
        debug_putchar(c);
        debug_print("'\n");
#endif
  }
}

/* =============================================================================
 * SECTION 7: Keyboard API
 * =============================================================================
 */

/*
 * keyboard_getchar - Get next character (blocking)
 *
 * Returns: Next character from keyboard (waits if none available)
 */
char keyboard_getchar(void) {
  while (kb_buffer_available() == 0) {
    hlt(); /* Wait for interrupt */
  }
  return kb_buffer_get();
}

/*
 * keyboard_getchar_nonblock - Get next character (non-blocking)
 *
 * Returns: Next character, or 0 if none available
 */
char keyboard_getchar_nonblock(void) { return kb_buffer_get(); }

/*
 * keyboard_get_last_key - Get the last key pressed
 *
 * Returns: The most recent key pressed (doesn't consume it)
 */
char keyboard_get_last_key(void) { return last_key; }

/*
 * keyboard_clear_last_key - Clear the last key
 */
void keyboard_clear_last_key(void) { last_key = 0; }

/*
 * keyboard_is_shift - Check if Shift is held
 */
bool keyboard_is_shift(void) { return shift_pressed; }

/*
 * keyboard_is_ctrl - Check if Ctrl is held
 */
bool keyboard_is_ctrl(void) { return ctrl_pressed; }

/*
 * keyboard_is_alt - Check if Alt is held
 */
bool keyboard_is_alt(void) { return alt_pressed; }
