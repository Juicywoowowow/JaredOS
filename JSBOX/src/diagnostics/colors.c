/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: ANSI Color Support Implementation
 */

#include "colors.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool g_colors_enabled = true;
static bool g_colors_checked = false;

bool jsbox_colors_supported(void) {
    /* Check if stdout is a terminal */
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }
    
    /* Check for NO_COLOR environment variable */
    const char* no_color = getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0') {
        return false;
    }
    
    /* Check TERM variable */
    const char* term = getenv("TERM");
    if (!term || strcmp(term, "dumb") == 0) {
        return false;
    }
    
    return true;
}

void jsbox_colors_enable(bool enable) {
    g_colors_enabled = enable;
    g_colors_checked = true;
}

bool jsbox_colors_enabled(void) {
    if (!g_colors_checked) {
        g_colors_enabled = jsbox_colors_supported();
        g_colors_checked = true;
    }
    return g_colors_enabled;
}

const char* jsbox_color(const char* color_code) {
    if (!jsbox_colors_enabled()) {
        return "";
    }
    return color_code;
}

const char* jsbox_style_error(void) {
    return jsbox_color(JSBOX_COLOR_BOLD JSBOX_COLOR_BRIGHT_RED);
}

const char* jsbox_style_warning(void) {
    return jsbox_color(JSBOX_COLOR_BOLD JSBOX_COLOR_BRIGHT_YELLOW);
}

const char* jsbox_style_note(void) {
    return jsbox_color(JSBOX_COLOR_BOLD JSBOX_COLOR_BRIGHT_CYAN);
}

const char* jsbox_style_hint(void) {
    return jsbox_color(JSBOX_COLOR_BOLD JSBOX_COLOR_BRIGHT_GREEN);
}

const char* jsbox_style_location(void) {
    return jsbox_color(JSBOX_COLOR_BOLD JSBOX_COLOR_BRIGHT_BLUE);
}

const char* jsbox_style_code(void) {
    return jsbox_color("");
}

const char* jsbox_style_highlight(void) {
    return jsbox_color(JSBOX_COLOR_BOLD JSBOX_COLOR_UNDERLINE);
}

const char* jsbox_style_reset(void) {
    return jsbox_color(JSBOX_COLOR_RESET);
}
