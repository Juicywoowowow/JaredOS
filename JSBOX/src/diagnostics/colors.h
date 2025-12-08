/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: ANSI Color Support
 */

#ifndef JSBOX_COLORS_H
#define JSBOX_COLORS_H

#include <stdbool.h>

/* ============================================================================
 * Color Configuration
 * ============================================================================ */

/* Check if terminal supports colors */
bool jsbox_colors_supported(void);

/* Enable/disable color output globally */
void jsbox_colors_enable(bool enable);

/* Check if colors are enabled */
bool jsbox_colors_enabled(void);

/* ============================================================================
 * ANSI Color Codes
 * ============================================================================ */

#define JSBOX_COLOR_RESET     "\x1b[0m"
#define JSBOX_COLOR_BOLD      "\x1b[1m"
#define JSBOX_COLOR_DIM       "\x1b[2m"
#define JSBOX_COLOR_ITALIC    "\x1b[3m"
#define JSBOX_COLOR_UNDERLINE "\x1b[4m"

/* Regular colors */
#define JSBOX_COLOR_BLACK     "\x1b[30m"
#define JSBOX_COLOR_RED       "\x1b[31m"
#define JSBOX_COLOR_GREEN     "\x1b[32m"
#define JSBOX_COLOR_YELLOW    "\x1b[33m"
#define JSBOX_COLOR_BLUE      "\x1b[34m"
#define JSBOX_COLOR_MAGENTA   "\x1b[35m"
#define JSBOX_COLOR_CYAN      "\x1b[36m"
#define JSBOX_COLOR_WHITE     "\x1b[37m"

/* Bright colors */
#define JSBOX_COLOR_BRIGHT_BLACK   "\x1b[90m"
#define JSBOX_COLOR_BRIGHT_RED     "\x1b[91m"
#define JSBOX_COLOR_BRIGHT_GREEN   "\x1b[92m"
#define JSBOX_COLOR_BRIGHT_YELLOW  "\x1b[93m"
#define JSBOX_COLOR_BRIGHT_BLUE    "\x1b[94m"
#define JSBOX_COLOR_BRIGHT_MAGENTA "\x1b[95m"
#define JSBOX_COLOR_BRIGHT_CYAN    "\x1b[96m"
#define JSBOX_COLOR_BRIGHT_WHITE   "\x1b[97m"

/* ============================================================================
 * Style Helpers
 * ============================================================================ */

/* Get color string (returns empty string if colors disabled) */
const char* jsbox_color(const char* color_code);

/* Styled text builders */
const char* jsbox_style_error(void);    /* Bold red */
const char* jsbox_style_warning(void);  /* Bold yellow */
const char* jsbox_style_note(void);     /* Bold cyan */
const char* jsbox_style_hint(void);     /* Bold green */
const char* jsbox_style_location(void); /* Bold blue */
const char* jsbox_style_code(void);     /* Normal */
const char* jsbox_style_highlight(void);/* Bold underline */
const char* jsbox_style_reset(void);    /* Reset */

#endif /* JSBOX_COLORS_H */
