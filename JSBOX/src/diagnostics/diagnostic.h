/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: Error/Warning Types
 */

#ifndef JSBOX_DIAGNOSTIC_H
#define JSBOX_DIAGNOSTIC_H

#include "source_location.h"
#include <stdbool.h>

/* ============================================================================
 * Diagnostic Levels
 * ============================================================================ */

typedef enum {
    JSBOX_DIAG_ERROR,    /* Fatal errors that stop execution */
    JSBOX_DIAG_WARNING,  /* Non-fatal warnings */
    JSBOX_DIAG_NOTE,     /* Additional context for errors */
    JSBOX_DIAG_HINT      /* Suggestions for fixes */
} JSBox_DiagLevel;

/* ============================================================================
 * Error Codes
 * ============================================================================ */

typedef enum {
    /* Lexer errors (E0001-E0099) */
    JSBOX_ERR_UNEXPECTED_CHAR       = 1,
    JSBOX_ERR_UNTERMINATED_STRING   = 2,
    JSBOX_ERR_UNTERMINATED_COMMENT  = 3,
    JSBOX_ERR_INVALID_NUMBER        = 4,
    JSBOX_ERR_INVALID_ESCAPE        = 5,
    
    /* Parser errors (E0100-E0199) */
    JSBOX_ERR_UNEXPECTED_TOKEN      = 100,
    JSBOX_ERR_EXPECTED_EXPRESSION   = 101,
    JSBOX_ERR_EXPECTED_STATEMENT    = 102,
    JSBOX_ERR_EXPECTED_IDENTIFIER   = 103,
    JSBOX_ERR_EXPECTED_SEMICOLON    = 104,
    JSBOX_ERR_EXPECTED_RPAREN       = 105,
    JSBOX_ERR_EXPECTED_RBRACE       = 106,
    JSBOX_ERR_EXPECTED_RBRACKET     = 107,
    JSBOX_ERR_EXPECTED_COLON        = 108,
    JSBOX_ERR_INVALID_ASSIGNMENT    = 109,
    JSBOX_ERR_DUPLICATE_PARAMETER   = 110,
    
    /* Runtime errors (E0200-E0299) */
    JSBOX_ERR_UNDEFINED_VARIABLE    = 200,
    JSBOX_ERR_NOT_A_FUNCTION        = 201,
    JSBOX_ERR_TYPE_ERROR            = 202,
    JSBOX_ERR_REFERENCE_ERROR       = 203,
    JSBOX_ERR_RANGE_ERROR           = 204,
    
    /* Sandbox errors (E0300-E0399) */
    JSBOX_ERR_PERMISSION_DENIED     = 300,
    JSBOX_ERR_FILE_NOT_FOUND        = 301,
    JSBOX_ERR_NETWORK_DISABLED      = 302
} JSBox_ErrorCode;

/* ============================================================================
 * Diagnostic Structure
 * ============================================================================ */

typedef struct JSBox_Diagnostic {
    JSBox_DiagLevel level;
    JSBox_ErrorCode code;
    char* message;
    JSBox_SourceSpan span;
    char* suggestion;          /* Optional fix suggestion */
    struct JSBox_Diagnostic* related;  /* Linked list of related notes */
} JSBox_Diagnostic;

/* ============================================================================
 * Diagnostic List
 * ============================================================================ */

typedef struct {
    JSBox_Diagnostic** items;
    size_t count;
    size_t capacity;
    size_t error_count;
    size_t warning_count;
} JSBox_DiagnosticList;

/* ============================================================================
 * Diagnostic API
 * ============================================================================ */

/* Create a new diagnostic */
JSBox_Diagnostic* jsbox_diagnostic_create(JSBox_DiagLevel level, JSBox_ErrorCode code,
                                           const char* message, JSBox_SourceSpan span);

/* Add a suggestion to diagnostic */
void jsbox_diagnostic_add_suggestion(JSBox_Diagnostic* diag, const char* suggestion);

/* Add a related note */
void jsbox_diagnostic_add_note(JSBox_Diagnostic* diag, const char* message, JSBox_SourceSpan span);

/* Destroy diagnostic */
void jsbox_diagnostic_destroy(JSBox_Diagnostic* diag);

/* ============================================================================
 * Diagnostic List API
 * ============================================================================ */

/* Create diagnostic list */
JSBox_DiagnosticList* jsbox_diag_list_create(void);

/* Destroy diagnostic list */
void jsbox_diag_list_destroy(JSBox_DiagnosticList* list);

/* Add diagnostic to list */
void jsbox_diag_list_add(JSBox_DiagnosticList* list, JSBox_Diagnostic* diag);

/* Quick helpers to add errors/warnings */
void jsbox_diag_list_error(JSBox_DiagnosticList* list, JSBox_ErrorCode code,
                            const char* message, JSBox_SourceSpan span);

void jsbox_diag_list_warning(JSBox_DiagnosticList* list, JSBox_ErrorCode code,
                              const char* message, JSBox_SourceSpan span);

/* Check if there are any errors */
bool jsbox_diag_list_has_errors(const JSBox_DiagnosticList* list);

/* Clear all diagnostics */
void jsbox_diag_list_clear(JSBox_DiagnosticList* list);

/* ============================================================================
 * Error Code Helpers
 * ============================================================================ */

/* Get error code string (e.g., "E0001") */
const char* jsbox_error_code_str(JSBox_ErrorCode code);

/* Get diagnostic level name */
const char* jsbox_diag_level_name(JSBox_DiagLevel level);

#endif /* JSBOX_DIAGNOSTIC_H */
