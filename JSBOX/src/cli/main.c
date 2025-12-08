/*
 * JSBOX - JavaScript Engine
 *
 * CLI: Main Entry Point
 */

#include "../base/memory.h"
#include "../diagnostics/colors.h"
#include "../diagnostics/reporter.h"
#include "../parsing/lexer.h"
#include "../parsing/parser.h"
#include "args.h"
#include "repl.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================================
 * File Reading
 * ============================================================================
 */

static char *read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Error: Could not open file '%s'\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)jsbox_malloc(size + 1);
  size_t read = fread(buffer, 1, size, file);
  buffer[read] = '\0';

  fclose(file);
  return buffer;
}

/* ============================================================================
 * Run Source Code
 * ============================================================================
 */

static int run_source(const char *source, const char *filename,
                      JSBox_Options *opts) {
  clock_t start_time = clock();

  /* Show tokens if requested */
  if (opts->show_tokens) {
    printf("\n%s=== Tokens ===%s\n\n", jsbox_style_location(),
           jsbox_style_reset());
    JSBox_Lexer *lexer = jsbox_lexer_create(source, filename);

    for (;;) {
      JSBox_Token tok = jsbox_lexer_next(lexer);
      if (tok.type == JSBOX_TOK_EOF)
        break;

      char text[64];
      jsbox_token_text(&tok, text, sizeof(text));
      printf("  %-15s '%s' at %zu:%zu\n", jsbox_token_type_name(tok.type), text,
             tok.span.start.line, tok.span.start.column);
      jsbox_token_free(&tok);
    }
    printf("\n");

    /* Report lexer errors */
    if (jsbox_lexer_has_errors(lexer)) {
      JSBox_Reporter *reporter =
          jsbox_reporter_create(jsbox_lexer_source_file(lexer));
      jsbox_reporter_emit_all(reporter, jsbox_lexer_diagnostics(lexer));
      jsbox_reporter_summary(reporter, jsbox_lexer_diagnostics(lexer));
      jsbox_reporter_destroy(reporter);
    }

    jsbox_lexer_destroy(lexer);
  }

  /* Parse */
  JSBox_Parser *parser = jsbox_parser_create(source, filename);
  JSBox_ASTNode *ast = jsbox_parser_parse(parser);

  /* Check for parse errors */
  if (jsbox_parser_has_errors(parser)) {
    JSBox_Reporter *reporter =
        jsbox_reporter_create(jsbox_parser_source_file(parser));
    jsbox_reporter_emit_all(reporter, jsbox_parser_diagnostics(parser));
    jsbox_reporter_summary(reporter, jsbox_parser_diagnostics(parser));
    jsbox_reporter_destroy(reporter);

    jsbox_ast_free(ast);
    jsbox_parser_destroy(parser);
    return 1;
  }

  /* Show AST if requested */
  if (opts->show_ast) {
    printf("\n%s=== AST ===%s\n\n", jsbox_style_location(),
           jsbox_style_reset());
    jsbox_ast_print(ast, 0);
    printf("\n");
  }

  /* TODO: Execute the AST when interpreter is implemented */
  if (!opts->show_tokens && !opts->show_ast) {
    printf("%s[Parsed successfully - execution not yet implemented]%s\n",
           jsbox_style_note(), jsbox_style_reset());
  }

  /* Show timing if requested */
  if (opts->show_time) {
    clock_t end_time = clock();
    double elapsed = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0;
    printf("\n%sExecution time: %.2f ms%s\n", jsbox_style_note(), elapsed,
           jsbox_style_reset());
  }

  jsbox_ast_free(ast);
  jsbox_parser_destroy(parser);

  return 0;
}

/* ============================================================================
 * Main
 * ============================================================================
 */

int main(int argc, char **argv) {
  JSBox_Options opts = jsbox_args_parse(argc, argv);

  /* Handle color setting */
  if (opts.no_colors) {
    jsbox_colors_enable(false);
  }

  /* Handle help/version */
  if (opts.help) {
    jsbox_args_print_help();
    return 0;
  }

  if (opts.version) {
    jsbox_args_print_version();
    return 0;
  }

  /* Handle -e eval */
  if (opts.eval_code) {
    return run_source(opts.eval_code, "<eval>", &opts);
  }

  /* Handle file */
  if (opts.filename) {
    char *source = read_file(opts.filename);
    if (!source) {
      return 1;
    }

    int result = run_source(source, opts.filename, &opts);
    jsbox_free(source);
    return result;
  }

  /* REPL mode */
  JSBox_REPLConfig repl_config = {.show_ast = opts.show_ast,
                                  .show_tokens = opts.show_tokens,
                                  .no_colors = opts.no_colors};
  jsbox_repl_run(repl_config);

  return 0;
}
