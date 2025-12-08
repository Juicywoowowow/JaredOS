/*
 * JSBOX - JavaScript Engine
 * 
 * Base: Logging and Debug Utilities
 */

#ifndef JSBOX_LOGGING_H
#define JSBOX_LOGGING_H

#include <stdio.h>
#include <stdbool.h>

/* ============================================================================
 * Log Levels
 * ============================================================================ */

typedef enum {
    JSBOX_LOG_TRACE,
    JSBOX_LOG_DEBUG,
    JSBOX_LOG_INFO,
    JSBOX_LOG_WARN,
    JSBOX_LOG_ERROR,
    JSBOX_LOG_FATAL
} JSBox_LogLevel;

/* ============================================================================
 * Logging Configuration
 * ============================================================================ */

/* Set minimum log level (default: JSBOX_LOG_INFO) */
void jsbox_log_set_level(JSBox_LogLevel level);

/* Get current log level */
JSBox_LogLevel jsbox_log_get_level(void);

/* Enable/disable colors in output */
void jsbox_log_set_colors(bool enabled);

/* Set output file (default: stderr) */
void jsbox_log_set_output(FILE* file);

/* ============================================================================
 * Logging Macros
 * ============================================================================ */

#ifdef JSBOX_DEBUG
    #define JSBOX_TRACE(...) jsbox_log(JSBOX_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
    #define JSBOX_DEBUG(...) jsbox_log(JSBOX_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
    #define JSBOX_TRACE(...) ((void)0)
    #define JSBOX_DEBUG(...) ((void)0)
#endif

#define JSBOX_INFO(...)  jsbox_log(JSBOX_LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define JSBOX_WARN(...)  jsbox_log(JSBOX_LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define JSBOX_ERROR(...) jsbox_log(JSBOX_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define JSBOX_FATAL(...) jsbox_log(JSBOX_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/* ============================================================================
 * Logging Functions
 * ============================================================================ */

/* Log a message (use macros instead) */
void jsbox_log(JSBox_LogLevel level, const char* file, int line, const char* fmt, ...);

/* ============================================================================
 * Assertions
 * ============================================================================ */

#ifdef JSBOX_DEBUG
    #define JSBOX_ASSERT(cond, msg)                                            \
        do {                                                                   \
            if (!(cond)) {                                                     \
                jsbox_assert_fail(#cond, msg, __FILE__, __LINE__);             \
            }                                                                  \
        } while (0)
#else
    #define JSBOX_ASSERT(cond, msg) ((void)0)
#endif

#define JSBOX_UNREACHABLE()                                                    \
    jsbox_unreachable(__FILE__, __LINE__)

/* Assertion failure handler */
void jsbox_assert_fail(const char* cond, const char* msg, const char* file, int line);

/* Unreachable code handler */
void jsbox_unreachable(const char* file, int line);

#endif /* JSBOX_LOGGING_H */
