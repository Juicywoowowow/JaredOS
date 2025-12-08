/*
 * JSBOX - JavaScript Engine
 * 
 * Parsing: Lexer (Tokenizer)
 */

#ifndef JSBOX_LEXER_H
#define JSBOX_LEXER_H

#include "tokens.h"
#include "../diagnostics/diagnostic.h"
#include "../diagnostics/source_location.h"

/* ============================================================================
 * Lexer Structure
 * ============================================================================ */

typedef struct {
    const char* source;          /* Full source text */
    const char* current;         /* Current position */
    const char* token_start;     /* Start of current token */
    
    size_t line;                 /* Current line (1-indexed) */
    size_t column;               /* Current column (1-indexed) */
    size_t offset;               /* Byte offset from start */
    
    size_t start_line;           /* Token start line */
    size_t start_column;         /* Token start column */
    size_t start_offset;         /* Token start offset */
    
    JSBox_SourceFile* source_file; /* For location tracking */
    JSBox_DiagnosticList* diagnostics;
} JSBox_Lexer;

/* ============================================================================
 * Lexer API
 * ============================================================================ */

/* Create a new lexer */
JSBox_Lexer* jsbox_lexer_create(const char* source, const char* filename);

/* Destroy lexer */
void jsbox_lexer_destroy(JSBox_Lexer* lexer);

/* Get the next token */
JSBox_Token jsbox_lexer_next(JSBox_Lexer* lexer);

/* Peek at the next token without consuming */
JSBox_Token jsbox_lexer_peek(JSBox_Lexer* lexer);

/* Get the source file (for error reporting) */
JSBox_SourceFile* jsbox_lexer_source_file(JSBox_Lexer* lexer);

/* Get diagnostics list */
JSBox_DiagnosticList* jsbox_lexer_diagnostics(JSBox_Lexer* lexer);

/* Check if lexer has errors */
bool jsbox_lexer_has_errors(const JSBox_Lexer* lexer);

#endif /* JSBOX_LEXER_H */
