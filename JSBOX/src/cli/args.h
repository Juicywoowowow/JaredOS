/*
 * JSBOX - JavaScript Engine
 *
 * CLI: Argument Parser
 */

#ifndef JSBOX_ARGS_H
#define JSBOX_ARGS_H

#include <stdbool.h>

/* ============================================================================
 * CLI Options
 * ============================================================================
 */

typedef struct {
  /* Input */
  const char *filename;  /* File to run (NULL for REPL) */
  const char *eval_code; /* -e "code" to evaluate */

  /* VM inspection flags */
  bool show_tokens; /* --show-tokens */
  bool show_ast;    /* --show-ast */
  bool show_memory; /* --show-memory */
  bool show_time;   /* --show-time */
  bool trace;       /* --trace */

  /* Output */
  bool no_colors; /* --no-colors */
  bool quiet;     /* --quiet */
  bool version;   /* --version */
  bool help;      /* --help */
} JSBox_Options;

/* ============================================================================
 * Argument Parsing
 * ============================================================================
 */

/* Parse command line arguments */
JSBox_Options jsbox_args_parse(int argc, char **argv);

/* Print help message */
void jsbox_args_print_help(void);

/* Print version */
void jsbox_args_print_version(void);

#endif /* JSBOX_ARGS_H */
