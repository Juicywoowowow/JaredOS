/*
 * =============================================================================
 * pong.c - Ping Pong Game for FoxOS
 * =============================================================================
 *
 * A classic Pong game to demonstrate FoxOS graphics capabilities.
 *
 * Controls:
 *   W/S - Move left paddle up/down
 *   Arrow Up/Down - Move right paddle up/down
 *   Space - Start game / Pause
 *
 * DEBUGGING TIPS:
 *   - If ball doesn't move, check timer is running
 *   - If paddles don't respond, verify keyboard handler
 *   - Ball physics: angle changes based on where it hits paddle
 *
 * =============================================================================
 */

#include "types.h"

/* External functions */
extern void vga_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                          uint8_t color);
extern void vga_draw_circle(int32_t cx, int32_t cy, int32_t r, uint8_t color);
extern void font_draw_string(int32_t x, int32_t y, const char *str, uint8_t fg,
                             uint8_t bg);
extern void font_draw_int(int32_t x, int32_t y, int32_t val, uint8_t fg,
                          uint8_t bg);
extern char keyboard_getchar_nonblock(void);
extern uint32_t timer_get_ticks(void);
extern int32_t window_create(const char *title, int32_t x, int32_t y, int32_t w,
                             int32_t h);
extern void window_set_content_callback(int32_t id,
                                        void (*cb)(int32_t, int32_t, int32_t,
                                                   int32_t));
extern bool window_is_visible(int32_t id);
extern void window_show(int32_t id);

/* =============================================================================
 * SECTION 1: Game Constants
 * =============================================================================
 */

#define GAME_WIDTH 200
#define GAME_HEIGHT 120

#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 24
#define PADDLE_SPEED 3

#define BALL_SIZE 4
#define BALL_SPEED 2

/* Colors */
#define COLOR_BG 0      /* Black */
#define COLOR_PADDLE 15 /* White */
#define COLOR_BALL 14   /* Yellow */
#define COLOR_TEXT 15   /* White */
#define COLOR_NET 8     /* Gray */

/* =============================================================================
 * SECTION 2: Game State
 * =============================================================================
 */

typedef struct {
  int32_t x, y;   /* Position */
  int32_t dx, dy; /* Velocity */
} ball_t;

typedef struct {
  int32_t y;     /* Y position (x is fixed) */
  int32_t score; /* Player score */
} paddle_t;

static ball_t ball;
static paddle_t left_paddle;
static paddle_t right_paddle;

static bool game_running = false;
static bool game_paused = true;
static int32_t pong_window_id = -1;
static uint32_t last_update_tick = 0;

/* =============================================================================
 * SECTION 3: Game Initialization
 * =============================================================================
 */

static void pong_reset_ball(void) {
  ball.x = GAME_WIDTH / 2;
  ball.y = GAME_HEIGHT / 2;
  ball.dx = (timer_get_ticks() % 2) ? BALL_SPEED : -BALL_SPEED;
  ball.dy = (timer_get_ticks() % 3) - 1; /* -1, 0, or 1 */
  if (ball.dy == 0)
    ball.dy = 1;
}

static void pong_reset_game(void) {
  left_paddle.y = GAME_HEIGHT / 2 - PADDLE_HEIGHT / 2;
  left_paddle.score = 0;

  right_paddle.y = GAME_HEIGHT / 2 - PADDLE_HEIGHT / 2;
  right_paddle.score = 0;

  pong_reset_ball();
  game_paused = true;
}

void pong_init(void) {
  pong_reset_game();
  game_running = true;
  last_update_tick = 0;

  /* Create pong window */
  pong_window_id =
      window_create("Pong", 50, 30, GAME_WIDTH + 4, GAME_HEIGHT + 4);

  debug_print("[PONG] Pong game initialized\n");
}

/* =============================================================================
 * SECTION 4: Game Input
 * =============================================================================
 */

static void pong_handle_input(void) {
  char key = keyboard_getchar_nonblock();

  while (key) {
    switch (key) {
    /* Left paddle - W/S keys */
    case 'w':
    case 'W':
      left_paddle.y -= PADDLE_SPEED;
      break;
    case 's':
    case 'S':
      left_paddle.y += PADDLE_SPEED;
      break;

    /* Right paddle - use I/K as arrow key alternatives */
    case 'i':
    case 'I':
      right_paddle.y -= PADDLE_SPEED;
      break;
    case 'k':
    case 'K':
      right_paddle.y += PADDLE_SPEED;
      break;

    /* Space to start/pause */
    case ' ':
      game_paused = !game_paused;
      break;

    /* R to reset */
    case 'r':
    case 'R':
      pong_reset_game();
      break;
    }

    key = keyboard_getchar_nonblock();
  }

  /* Clamp paddle positions */
  if (left_paddle.y < 0)
    left_paddle.y = 0;
  if (left_paddle.y > GAME_HEIGHT - PADDLE_HEIGHT) {
    left_paddle.y = GAME_HEIGHT - PADDLE_HEIGHT;
  }

  if (right_paddle.y < 0)
    right_paddle.y = 0;
  if (right_paddle.y > GAME_HEIGHT - PADDLE_HEIGHT) {
    right_paddle.y = GAME_HEIGHT - PADDLE_HEIGHT;
  }
}

/* =============================================================================
 * SECTION 5: Game Physics
 * =============================================================================
 */

static void pong_update_ball(void) {
  /* Move ball */
  ball.x += ball.dx;
  ball.y += ball.dy;

  /* Bounce off top/bottom walls */
  if (ball.y <= 0 || ball.y >= GAME_HEIGHT - BALL_SIZE) {
    ball.dy = -ball.dy;
    if (ball.y < 0)
      ball.y = 0;
    if (ball.y > GAME_HEIGHT - BALL_SIZE)
      ball.y = GAME_HEIGHT - BALL_SIZE;
  }

  /* Check left paddle collision */
  if (ball.x <= PADDLE_WIDTH + 4) {
    if (ball.y + BALL_SIZE >= left_paddle.y &&
        ball.y <= left_paddle.y + PADDLE_HEIGHT) {
      ball.dx = BALL_SPEED;
      /* Adjust angle based on where ball hits paddle */
      int32_t paddle_center = left_paddle.y + PADDLE_HEIGHT / 2;
      int32_t ball_center = ball.y + BALL_SIZE / 2;
      ball.dy = (ball_center - paddle_center) / 4;
      if (ball.dy == 0)
        ball.dy = (timer_get_ticks() % 2) ? 1 : -1;
    } else if (ball.x < 0) {
      /* Left player missed - right scores */
      right_paddle.score++;
      pong_reset_ball();
    }
  }

  /* Check right paddle collision */
  if (ball.x >= GAME_WIDTH - PADDLE_WIDTH - 4 - BALL_SIZE) {
    if (ball.y + BALL_SIZE >= right_paddle.y &&
        ball.y <= right_paddle.y + PADDLE_HEIGHT) {
      ball.dx = -BALL_SPEED;
      int32_t paddle_center = right_paddle.y + PADDLE_HEIGHT / 2;
      int32_t ball_center = ball.y + BALL_SIZE / 2;
      ball.dy = (ball_center - paddle_center) / 4;
      if (ball.dy == 0)
        ball.dy = (timer_get_ticks() % 2) ? 1 : -1;
    } else if (ball.x > GAME_WIDTH) {
      /* Right player missed - left scores */
      left_paddle.score++;
      pong_reset_ball();
    }
  }
}

/* =============================================================================
 * SECTION 6: Game Update
 * =============================================================================
 */

void pong_update(void) {
  if (!game_running)
    return;
  if (!window_is_visible(pong_window_id))
    return;

  pong_handle_input();

  /* Update at fixed rate (every 2 ticks at 100Hz = 50 updates/sec) */
  uint32_t current_tick = timer_get_ticks();
  if (current_tick - last_update_tick < 2)
    return;
  last_update_tick = current_tick;

  if (!game_paused) {
    pong_update_ball();
  }
}

/* =============================================================================
 * SECTION 7: Game Rendering
 * =============================================================================
 */

/*
 * pong_draw_content - Draw game inside window
 * Called by window manager
 */
void pong_draw_content(int32_t x, int32_t y, int32_t w, int32_t h) {
  /* Clear game area */
  vga_draw_rect(x, y, w, h, COLOR_BG);

  /* Draw center net */
  for (int32_t ny = 0; ny < h; ny += 8) {
    vga_draw_rect(x + w / 2 - 1, y + ny, 2, 4, COLOR_NET);
  }

  /* Draw paddles */
  vga_draw_rect(x + 4, y + left_paddle.y, PADDLE_WIDTH, PADDLE_HEIGHT,
                COLOR_PADDLE);
  vga_draw_rect(x + w - PADDLE_WIDTH - 4, y + right_paddle.y, PADDLE_WIDTH,
                PADDLE_HEIGHT, COLOR_PADDLE);

  /* Draw ball */
  if (!game_paused) {
    vga_draw_rect(x + ball.x, y + ball.y, BALL_SIZE, BALL_SIZE, COLOR_BALL);
  }

  /* Draw scores */
  font_draw_int(x + w / 2 - 30, y + 4, left_paddle.score, COLOR_TEXT, COLOR_BG);
  font_draw_int(x + w / 2 + 20, y + 4, right_paddle.score, COLOR_TEXT,
                COLOR_BG);

  /* Draw pause message */
  if (game_paused) {
    font_draw_string(x + w / 2 - 40, y + h / 2 - 4, "SPACE=Start", COLOR_TEXT,
                     COLOR_BG);
    font_draw_string(x + w / 2 - 40, y + h / 2 + 8, "W/S   I/K", COLOR_NET,
                     COLOR_BG);
  }
}

/*
 * pong_get_window_id - Get the pong window ID
 */
int32_t pong_get_window_id(void) { return pong_window_id; }
