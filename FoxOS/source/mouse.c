/*
 * =============================================================================
 * mouse.c - PS/2 Mouse Driver for FoxOS
 * =============================================================================
 *
 * This driver handles the PS/2 mouse controller. It:
 *   1. Initializes the mouse through the keyboard controller
 *   2. Reads mouse packets (3 bytes: buttons + X/Y deltas)
 *   3. Tracks cursor position within screen bounds
 *   4. Provides button state information
 *
 * The PS/2 mouse connects through the keyboard controller (8042).
 * We need to send commands via port 0x64 and read/write data via 0x60.
 *
 * DEBUGGING TIPS:
 *   - If mouse doesn't work, the 8042 might not support PS/2 mouse
 *   - Check IRQ12 is properly set up in the IDT
 *   - Mouse packets are 3 bytes - missing bytes = wrong cursor movement
 *   - Use debug_print to output packet bytes for debugging
 *
 * =============================================================================
 */

#include "types.h"

/* =============================================================================
 * SECTION 1: PS/2 Controller Ports and Constants
 * =============================================================================
 */

#define PS2_DATA_PORT 0x60   /* Data port */
#define PS2_STATUS_PORT 0x64 /* Status port (read) */
#define PS2_CMD_PORT 0x64    /* Command port (write) */

/* Status register bits */
#define PS2_STATUS_OUTPUT 0x01 /* Output buffer full */
#define PS2_STATUS_INPUT 0x02  /* Input buffer full */

/* Controller commands (sent to 0x64) */
#define PS2_CMD_READ_CONFIG 0x20  /* Read controller config byte */
#define PS2_CMD_WRITE_CONFIG 0x60 /* Write controller config byte */
#define PS2_CMD_DISABLE_AUX 0xA7  /* Disable auxiliary (mouse) port */
#define PS2_CMD_ENABLE_AUX 0xA8   /* Enable auxiliary port */
#define PS2_CMD_WRITE_AUX 0xD4    /* Write next byte to mouse */

/* Mouse commands (sent to 0x60 after 0xD4) */
#define MOUSE_CMD_SET_DEFAULTS 0xF6 /* Set default settings */
#define MOUSE_CMD_ENABLE 0xF4       /* Enable data reporting */
#define MOUSE_CMD_DISABLE 0xF5      /* Disable data reporting */
#define MOUSE_CMD_SET_RATE 0xF3     /* Set sample rate */
#define MOUSE_CMD_GET_ID 0xF2       /* Get device ID */

/* Mouse packet byte 0 bits */
#define MOUSE_LEFT_BTN 0x01   /* Left button pressed */
#define MOUSE_RIGHT_BTN 0x02  /* Right button pressed */
#define MOUSE_MIDDLE_BTN 0x04 /* Middle button pressed */
#define MOUSE_ALWAYS_1 0x08   /* Always set (packet sync) */
#define MOUSE_X_SIGN 0x10     /* X movement is negative */
#define MOUSE_Y_SIGN 0x20     /* Y movement is negative */
#define MOUSE_X_OVERFLOW 0x40 /* X movement overflow */
#define MOUSE_Y_OVERFLOW 0x80 /* Y movement overflow */

/* =============================================================================
 * SECTION 2: Mouse State
 * =============================================================================
 */

/* Screen boundaries for cursor (VGA Mode 13h: 320x200) */
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

/* Cursor position */
static int32_t mouse_x = SCREEN_WIDTH / 2;
static int32_t mouse_y = SCREEN_HEIGHT / 2;

/* Button states */
static bool mouse_left = false;
static bool mouse_right = false;
static bool mouse_middle = false;

/* Packet parsing state (mouse sends 3-byte packets) */
static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index = 0;

/* =============================================================================
 * SECTION 3: PS/2 Controller Communication Helpers
 * =============================================================================
 */

/*
 * ps2_wait_input - Wait until we can write to the PS/2 controller
 */
static void ps2_wait_input(void) {
  int timeout = 100000;
  while ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT) && --timeout) {
    io_wait();
  }
}

/*
 * ps2_wait_output - Wait until we can read from the PS/2 controller
 */
static void ps2_wait_output(void) {
  int timeout = 100000;
  while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT) && --timeout) {
    io_wait();
  }
}

/*
 * ps2_send_command - Send a command to the PS/2 controller
 */
static void ps2_send_command(uint8_t cmd) {
  ps2_wait_input();
  outb(PS2_CMD_PORT, cmd);
}

/*
 * ps2_send_data - Send data to the PS/2 data port
 */
static void ps2_send_data(uint8_t data) {
  ps2_wait_input();
  outb(PS2_DATA_PORT, data);
}

/*
 * ps2_read_data - Read data from the PS/2 data port
 */
static uint8_t ps2_read_data(void) {
  ps2_wait_output();
  return inb(PS2_DATA_PORT);
}

/*
 * mouse_send_command - Send a command to the mouse
 *
 * @param cmd: Mouse command byte
 * Returns: Acknowledgement byte (should be 0xFA)
 */
static uint8_t mouse_send_command(uint8_t cmd) {
  /* Tell controller the next byte is for the mouse */
  ps2_send_command(PS2_CMD_WRITE_AUX);
  ps2_send_data(cmd);
  return ps2_read_data(); /* Read ACK */
}

/* =============================================================================
 * SECTION 4: Mouse Initialization
 * =============================================================================
 */

/*
 * mouse_init - Initialize the PS/2 mouse
 *
 * This enables the auxiliary (mouse) port and configures the mouse
 * to send movement packets.
 */
void mouse_init(void) {
  uint8_t status;

  debug_print("[MOUSE] Initializing PS/2 mouse...\n");

  /* Enable the auxiliary (mouse) port */
  ps2_send_command(PS2_CMD_ENABLE_AUX);

  /* Get the current controller configuration byte */
  ps2_send_command(PS2_CMD_READ_CONFIG);
  status = ps2_read_data();

  /* Enable IRQ12 (auxiliary device interrupt) */
  status |= 0x02;  /* Enable aux interrupt */
  status &= ~0x20; /* Enable aux clock */

  /* Write the updated configuration */
  ps2_send_command(PS2_CMD_WRITE_CONFIG);
  ps2_send_data(status);

  /* Set mouse to default settings */
  mouse_send_command(MOUSE_CMD_SET_DEFAULTS);
  debug_print("[MOUSE] Set defaults: ");
  debug_hex(ps2_read_data());
  debug_print("\n");

  /* Set sample rate to 100 samples/second */
  mouse_send_command(MOUSE_CMD_SET_RATE);
  ps2_send_data(100);

  /* Enable data reporting */
  mouse_send_command(MOUSE_CMD_ENABLE);
  debug_print("[MOUSE] Enabled data reporting\n");

  /* Initialize position to center of screen */
  mouse_x = SCREEN_WIDTH / 2;
  mouse_y = SCREEN_HEIGHT / 2;

  debug_print("[MOUSE] Mouse initialized\n");
  debug_print("[MOUSE] Initial position: (");
  debug_hex(mouse_x);
  debug_print(", ");
  debug_hex(mouse_y);
  debug_print(")\n");
}

/* =============================================================================
 * SECTION 5: Mouse Interrupt Handler
 * =============================================================================
 */

/*
 * mouse_handler - Called on mouse interrupt (IRQ12)
 *
 * Reads mouse packets and updates cursor position and button states.
 * Called from irq_handler in kernelA.c.
 */
void mouse_handler(void) {
  uint8_t data = inb(PS2_DATA_PORT);

  /* Store byte in packet buffer */
  mouse_packet[mouse_packet_index++] = data;

  /* Check for packet sync (bit 3 of first byte should always be 1) */
  if (mouse_packet_index == 1 && !(data & MOUSE_ALWAYS_1)) {
    /* Out of sync - reset */
    mouse_packet_index = 0;
    return;
  }

  /* Wait for complete 3-byte packet */
  if (mouse_packet_index < 3) {
    return;
  }

  /* Reset packet index for next packet */
  mouse_packet_index = 0;

  /* Parse packet */
  uint8_t status = mouse_packet[0];
  int8_t x_rel = mouse_packet[1];
  int8_t y_rel = mouse_packet[2];

  /* Handle overflow (ignore packet if overflow) */
  if ((status & MOUSE_X_OVERFLOW) || (status & MOUSE_Y_OVERFLOW)) {
    return;
  }

  /* Apply sign extension for negative movement */
  if (status & MOUSE_X_SIGN) {
    x_rel = x_rel | 0xFFFFFF00; /* Sign extend */
  }
  if (status & MOUSE_Y_SIGN) {
    y_rel = y_rel | 0xFFFFFF00; /* Sign extend */
  }

  /* Update cursor position */
  /* Note: Mouse Y is inverted (positive = up), but screen Y increases downward
   */
  mouse_x += x_rel;
  mouse_y -= y_rel; /* Invert Y */

  /* Clamp to screen boundaries */
  if (mouse_x < 0)
    mouse_x = 0;
  if (mouse_x >= SCREEN_WIDTH)
    mouse_x = SCREEN_WIDTH - 1;
  if (mouse_y < 0)
    mouse_y = 0;
  if (mouse_y >= SCREEN_HEIGHT)
    mouse_y = SCREEN_HEIGHT - 1;

  /* Update button states */
  mouse_left = (status & MOUSE_LEFT_BTN) != 0;
  mouse_right = (status & MOUSE_RIGHT_BTN) != 0;
  mouse_middle = (status & MOUSE_MIDDLE_BTN) != 0;

/* Debug output */
#if 0
    debug_print("[MOUSE] Pos: (");
    debug_hex(mouse_x);
    debug_print(", ");
    debug_hex(mouse_y);
    debug_print(") BTN: ");
    if (mouse_left) debug_print("L");
    if (mouse_middle) debug_print("M");
    if (mouse_right) debug_print("R");
    debug_print("\n");
#endif
}

/* =============================================================================
 * SECTION 6: Mouse API
 * =============================================================================
 */

/*
 * mouse_get_x - Get current mouse X position
 */
int32_t mouse_get_x(void) { return mouse_x; }

/*
 * mouse_get_y - Get current mouse Y position
 */
int32_t mouse_get_y(void) { return mouse_y; }

/*
 * mouse_get_position - Get current mouse position
 *
 * @param x: Pointer to store X position
 * @param y: Pointer to store Y position
 */
void mouse_get_position(int32_t *x, int32_t *y) {
  *x = mouse_x;
  *y = mouse_y;
}

/*
 * mouse_is_left_pressed - Check if left button is pressed
 */
bool mouse_is_left_pressed(void) { return mouse_left; }

/*
 * mouse_is_right_pressed - Check if right button is pressed
 */
bool mouse_is_right_pressed(void) { return mouse_right; }

/*
 * mouse_is_middle_pressed - Check if middle button is pressed
 */
bool mouse_is_middle_pressed(void) { return mouse_middle; }

/*
 * mouse_set_position - Set mouse cursor position
 *
 * @param x: New X position
 * @param y: New Y position
 */
void mouse_set_position(int32_t x, int32_t y) {
  /* Clamp to screen boundaries */
  if (x < 0)
    x = 0;
  if (x >= SCREEN_WIDTH)
    x = SCREEN_WIDTH - 1;
  if (y < 0)
    y = 0;
  if (y >= SCREEN_HEIGHT)
    y = SCREEN_HEIGHT - 1;

  mouse_x = x;
  mouse_y = y;
}
