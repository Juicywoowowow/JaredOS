/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: Pretty Error Reporting
 * 
 * Prints Rust-style error messages with source context:
 * 
 * error[E0001]: Unexpected token
 *   --> script.js:5:10
 *    |
 *  4 |   if (x > 5)
 *  5 |   else {
 *    |   ^^^^ unexpected 'else' here
 *    |
 *    = help: did you mean to add a statement before 'else'?
 */

#ifndef JSBOX_REPORTER_H
#define JSBOX_REPORTER_H

#include "diagnostic.h"
#include "source_location.h"
#include <stdio.h>

/* ============================================================================
 * Reporter Configuration
 * ============================================================================ */

typedef struct {
    FILE* output;              /* Output stream (default: stderr) */
    bool colors;               /* Use ANSI colors */
    size_t context_lines;      /* Lines of context to show (default: 1) */
    bool show_line_numbers;    /* Show line numbers (default: true) */
    bool show_error_codes;     /* Show error codes (default: true) */
    bool compact;              /* Compact output mode */
} JSBox_ReporterConfig;

typedef struct {
    JSBox_ReporterConfig config;
    const JSBox_SourceFile* source;
} JSBox_Reporter;

/* ============================================================================
 * Reporter API
 * ============================================================================ */

/* Create reporter with default config */
JSBox_Reporter* jsbox_reporter_create(const JSBox_SourceFile* source);

/* Create reporter with custom config */
JSBox_Reporter* jsbox_reporter_create_with_config(const JSBox_SourceFile* source,
                                                   JSBox_ReporterConfig config);

/* Destroy reporter */
void jsbox_reporter_destroy(JSBox_Reporter* reporter);

/* Get default configuration */
JSBox_ReporterConfig jsbox_reporter_default_config(void);

/* ============================================================================
 * Reporting Functions
 * ============================================================================ */

/* Report a single diagnostic */
void jsbox_reporter_emit(JSBox_Reporter* reporter, const JSBox_Diagnostic* diag);

/* Report all diagnostics in a list */
void jsbox_reporter_emit_all(JSBox_Reporter* reporter, const JSBox_DiagnosticList* list);

/* Print summary (e.g., "1 error, 2 warnings") */
void jsbox_reporter_summary(JSBox_Reporter* reporter, const JSBox_DiagnosticList* list);

#endif /* JSBOX_REPORTER_H */
