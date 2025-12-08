/*
 * JSBOX - JavaScript Engine
 *
 * CLI: Argument Parser Implementation
 */

#include "args.h"
#include <stdio.h>
#include <string.h>

#define JSBOX_VERSION "0.1.0"

JSBox_Options jsbox_args_parse(int argc, char **argv) {
  JSBox_Options opts = {0};

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];

    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      opts.help = true;
    } else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
      opts.version = true;
    } else if (strcmp(arg, "--show-tokens") == 0) {
      opts.show_tokens = true;
    } else if (strcmp(arg, "--show-ast") == 0) {
      opts.show_ast = true;
    } else if (strcmp(arg, "--show-memory") == 0) {
      opts.show_memory = true;
    } else if (strcmp(arg, "--show-time") == 0) {
      opts.show_time = true;
    } else if (strcmp(arg, "--trace") == 0) {
      opts.trace = true;
    } else if (strcmp(arg, "--no-colors") == 0) {
      opts.no_colors = true;
    } else if (strcmp(arg, "--quiet") == 0 || strcmp(arg, "-q") == 0) {
      opts.quiet = true;
    } else if (strcmp(arg, "-e") == 0 || strcmp(arg, "--eval") == 0) {
      if (i + 1 < argc) {
        opts.eval_code = argv[++i];
      } else {
        fprintf(stderr, "Error: -e requires an argument\n");
      }
    } else if (arg[0] == '-') {
      fprintf(stderr, "Unknown option: %s\n", arg);
    } else {
      /* Positional argument - file to run */
      opts.filename = arg;
    }
  }

  return opts;
}

void jsbox_args_print_help(void) {
  printf("JSBOX - JavaScript Engine v%s\n"
         "\n"
         "Usage: jbox [options] [file.js]\n"
         "\n"
         "Options:\n"
         "  -h, --help        Show this help message\n"
         "  -v, --version     Show version\n"
         "  -e, --eval CODE   Evaluate JavaScript code\n"
         "  -q, --quiet       Suppress output\n"
         "  --no-colors       Disable colored output\n"
         "\n"
         "VM Inspection:\n"
         "  --show-tokens     Print tokens (lexer output)\n"
         "  --show-ast        Print AST (parser output)\n"
         "  --show-memory     Show memory usage after execution\n"
         "  --show-time       Show execution time\n"
         "  --trace           Trace execution step-by-step\n"
         "\n"
         "Examples:\n"
         "  jbox script.js                 Run a JavaScript file\n"
         "  jbox                           Start interactive REPL\n"
         "  jbox -e 'console.log(1+2)'     Evaluate inline code\n"
         "  jbox --show-ast script.js      Show AST of a file\n"
         "\n",
         JSBOX_VERSION);
}

void jsbox_args_print_version(void) { printf("JSBOX v%s\n", JSBOX_VERSION); }
