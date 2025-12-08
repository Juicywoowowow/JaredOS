/*
 * JSBOX - JavaScript Engine
 *
 * Parsing: Pratt Parser
 *
 * Uses Pratt parsing (precedence climbing) for expressions
 * and recursive descent for statements.
 */

#ifndef JSBOX_PARSER_H
#define JSBOX_PARSER_H

#include "../diagnostics/diagnostic.h"
#include "ast.h"
#include "lexer.h"

/* ============================================================================
 * Parser Structure
 * ============================================================================
 */

typedef struct {
  JSBox_Lexer *lexer;
  JSBox_Token current;
  JSBox_Token previous;
  JSBox_DiagnosticList *diagnostics;
  bool had_error;
  bool panic_mode;
} JSBox_Parser;

/* ============================================================================
 * Parser API
 * ============================================================================
 */

/* Create a new parser */
JSBox_Parser *jsbox_parser_create(const char *source, const char *filename);

/* Destroy parser */
void jsbox_parser_destroy(JSBox_Parser *parser);

/* Parse the source into an AST */
JSBox_ASTNode *jsbox_parser_parse(JSBox_Parser *parser);

/* Parse a single expression */
JSBox_ASTNode *jsbox_parser_parse_expression(JSBox_Parser *parser);

/* Get diagnostics list */
JSBox_DiagnosticList *jsbox_parser_diagnostics(JSBox_Parser *parser);

/* Get source file (for error reporting) */
JSBox_SourceFile *jsbox_parser_source_file(JSBox_Parser *parser);

/* Check if parser has errors */
bool jsbox_parser_has_errors(const JSBox_Parser *parser);

#endif /* JSBOX_PARSER_H */
