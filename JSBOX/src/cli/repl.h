/*
 * JSBOX - JavaScript Engine
 *
 * CLI: REPL (Read-Eval-Print Loop)
 */

#ifndef JSBOX_REPL_H
#define JSBOX_REPL_H

#include "../diagnostics/reporter.h"
#include "../parsing/parser.h"

/* ============================================================================
 * REPL
 * ============================================================================
 */

typedef struct {
  bool show_ast;
  bool show_tokens;
  bool no_colors;
} JSBox_REPLConfig;

/* Run the REPL */
void jsbox_repl_run(JSBox_REPLConfig config);

#endif /* JSBOX_REPL_H */
