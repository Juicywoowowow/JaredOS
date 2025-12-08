/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Logging Implementation
 */

#include "logging.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * Global State
 * ============================================================================ */

static JSBox_LogLevel g_log_level = JSBOX_LOG_INFO;
static bool g_log_colors = true;
static FILE* g_log_output = NULL;

/* ============================================================================
 * ANSI Colors
 * ============================================================================ */

#define ANSI_RESET   "\x1b[0m"
#define ANSI_GRAY    "\x1b[90m"
#define ANSI_RED     "\x1b[91m"
#define ANSI_GREEN   "\x1b[92m"
#define ANSI_YELLOW  "\x1b[93m"
#define ANSI_BLUE    "\x1b[94m"
#define ANSI_MAGENTA "\x1b[95m"
#define ANSI_CYAN    "\x1b[96m"
#define ANSI_BOLD    "\x1b[1m"

static const char* level_colors[] = {
    ANSI_GRAY,     /* TRACE */
    ANSI_CYAN,     /* DEBUG */
    ANSI_GREEN,    /* INFO */
    ANSI_YELLOW,   /* WARN */
    ANSI_RED,      /* ERROR */
    ANSI_BOLD ANSI_RED  /* FATAL */
};

static const char* level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

/* ============================================================================
 * Configuration
 * ============================================================================ */

void jsbox_log_set_level(JSBox_LogLevel level) {
    g_log_level = level;
}

JSBox_LogLevel jsbox_log_get_level(void) {
    return g_log_level;
}

void jsbox_log_set_colors(bool enabled) {
    g_log_colors = enabled;
}

void jsbox_log_set_output(FILE* file) {
    g_log_output = file;
}

/* ============================================================================
 * Logging
 * ============================================================================ */

void jsbox_log(JSBox_LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (level < g_log_level) return;
    
    FILE* out = g_log_output ? g_log_output : stderr;
    
    /* Get just the filename, not full path */
    const char* filename = strrchr(file, '/');
    if (filename) {
        filename++;
    } else {
        filename = file;
    }
    
    /* Timestamp */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
    
    /* Print log line */
    if (g_log_colors) {
        fprintf(out, "%s%s%s %s[%s]%s %s:%d: ",
                ANSI_GRAY, time_buf, ANSI_RESET,
                level_colors[level], level_names[level], ANSI_RESET,
                filename, line);
    } else {
        fprintf(out, "%s [%s] %s:%d: ",
                time_buf, level_names[level], filename, line);
    }
    
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
    
    fprintf(out, "\n");
    fflush(out);
    
    /* Fatal errors abort */
    if (level == JSBOX_LOG_FATAL) {
        abort();
    }
}

/* ============================================================================
 * Assertions
 * ============================================================================ */

void jsbox_assert_fail(const char* cond, const char* msg, const char* file, int line) {
    fprintf(stderr, "\n");
    fprintf(stderr, "%s=== JSBOX ASSERTION FAILED ===%s\n", ANSI_BOLD ANSI_RED, ANSI_RESET);
    fprintf(stderr, "  Condition: %s\n", cond);
    fprintf(stderr, "  Message:   %s\n", msg);
    fprintf(stderr, "  Location:  %s:%d\n", file, line);
    fprintf(stderr, "\n");
    abort();
}

void jsbox_unreachable(const char* file, int line) {
    fprintf(stderr, "\n");
    fprintf(stderr, "%s=== JSBOX UNREACHABLE CODE ===%s\n", ANSI_BOLD ANSI_RED, ANSI_RESET);
    fprintf(stderr, "  Location: %s:%d\n", file, line);
    fprintf(stderr, "\n");
    abort();
}
