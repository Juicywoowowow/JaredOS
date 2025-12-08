/*
 * JSBOX - JavaScript Engine
 *
 * CLI: REPL Implementation
 */

#include "repl.h"
#include "../base/strings.h"
#include "../diagnostics/colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPL_LINE_SIZE 4096

void jsbox_repl_run(JSBox_REPLConfig config) {
  char line[REPL_LINE_SIZE];

  if (config.no_colors) {
    jsbox_colors_enable(false);
  }

  printf("%sJSBOX%s JavaScript Engine - Interactive Mode\n",
         jsbox_style_location(), jsbox_style_reset());
  printf("Type '.help' for help, '.exit' to quit\n\n");

  for (;;) {
    printf("%s> %s", jsbox_style_hint(), jsbox_style_reset());
    fflush(stdout);

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    /* Remove trailing newline */
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
      len--;
    }

    /* Skip empty lines */
    if (len == 0)
      continue;

    /* Check for REPL commands */
    if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
      break;
    }
    if (strcmp(line, ".help") == 0) {
      printf("REPL Commands:\n"
             "  .help     Show this help\n"
             "  .exit     Exit the REPL\n"
             "  .ast      Toggle AST display\n"
             "  .tokens   Toggle token display\n"
             "\n");
      continue;
    }
    if (strcmp(line, ".ast") == 0) {
      config.show_ast = !config.show_ast;
      printf("AST display: %s\n", config.show_ast ? "on" : "off");
      continue;
    }
    if (strcmp(line, ".tokens") == 0) {
      config.show_tokens = !config.show_tokens;
      printf("Token display: %s\n", config.show_tokens ? "on" : "off");
      continue;
    }

    /* Parse the input */
    JSBox_Parser *parser = jsbox_parser_create(line, "<repl>");

    /* Show tokens if requested */
    if (config.show_tokens) {
      printf("\n--- Tokens ---\n");
      JSBox_Lexer *lexer = jsbox_lexer_create(line, "<repl>");
      for (;;) {
        JSBox_Token tok = jsbox_lexer_next(lexer);
        if (tok.type == JSBOX_TOK_EOF)
          break;

        char text[64];
        jsbox_token_text(&tok, text, sizeof(text));
        printf("  %s: '%s' at %zu:%zu\n", jsbox_token_type_name(tok.type), text,
               tok.span.start.line, tok.span.start.column);
        jsbox_token_free(&tok);
      }
      jsbox_lexer_destroy(lexer);
      printf("\n");
    }

    /* Parse */
    JSBox_ASTNode *ast = jsbox_parser_parse(parser);

    /* Report errors */
    if (jsbox_parser_has_errors(parser)) {
      JSBox_Reporter *reporter =
          jsbox_reporter_create(jsbox_parser_source_file(parser));
      jsbox_reporter_emit_all(reporter, jsbox_parser_diagnostics(parser));
      jsbox_reporter_destroy(reporter);
    } else {
      /* Show AST if requested */
      if (config.show_ast) {
        printf("\n--- AST ---\n");
        jsbox_ast_print(ast, 0);
        printf("\n");
      }

      /* TODO: Execute the AST */
      printf("%s[Parsed OK - execution not yet implemented]%s\n",
             jsbox_style_note(), jsbox_style_reset());
    }

    jsbox_ast_free(ast);
    jsbox_parser_destroy(parser);
  }

  printf("Goodbye!\n");
}
