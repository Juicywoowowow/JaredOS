/*
 * JSBOX - JavaScript Engine
 *
 * Diagnostics: Pretty Error Reporting Implementation
 */

#include "reporter.h"
#include "../base/memory.h"
#include "colors.h"
#include <string.h>

/* ============================================================================
 * Reporter Creation
 * ============================================================================
 */

JSBox_ReporterConfig jsbox_reporter_default_config(void) {
  JSBox_ReporterConfig config = {.output = NULL, /* Will use stderr */
                                 .colors = true,
                                 .context_lines = 1,
                                 .show_line_numbers = true,
                                 .show_error_codes = true,
                                 .compact = false};
  return config;
}

JSBox_Reporter *jsbox_reporter_create(const JSBox_SourceFile *source) {
  return jsbox_reporter_create_with_config(source,
                                           jsbox_reporter_default_config());
}

JSBox_Reporter *
jsbox_reporter_create_with_config(const JSBox_SourceFile *source,
                                  JSBox_ReporterConfig config) {
  JSBox_Reporter *reporter =
      (JSBox_Reporter *)jsbox_malloc(sizeof(JSBox_Reporter));
  reporter->config = config;
  reporter->source = source;

  if (!reporter->config.output) {
    reporter->config.output = stderr;
  }

  /* Auto-detect color support if colors enabled */
  if (reporter->config.colors) {
    reporter->config.colors = jsbox_colors_enabled();
  }

  return reporter;
}

void jsbox_reporter_destroy(JSBox_Reporter *reporter) { jsbox_free(reporter); }

/* ============================================================================
 * Internal Helpers
 * ============================================================================
 */

static const char *get_level_style(JSBox_DiagLevel level) {
  switch (level) {
  case JSBOX_DIAG_ERROR:
    return jsbox_style_error();
  case JSBOX_DIAG_WARNING:
    return jsbox_style_warning();
  case JSBOX_DIAG_NOTE:
    return jsbox_style_note();
  case JSBOX_DIAG_HINT:
    return jsbox_style_hint();
  default:
    return jsbox_style_reset();
  }
}

static void print_line_gutter(FILE *out, size_t line_num, int width,
                              bool colors) {
  if (line_num > 0) {
    if (colors) {
      fprintf(out, "%s%*zu |%s ", jsbox_style_location(), width, line_num,
              jsbox_style_reset());
    } else {
      fprintf(out, "%*zu | ", width, line_num);
    }
  } else {
    if (colors) {
      fprintf(out, "%s%*s |%s ", jsbox_style_location(), width, "",
              jsbox_style_reset());
    } else {
      fprintf(out, "%*s | ", width, "");
    }
  }
}

static void print_caret_line(FILE *out, size_t column, size_t length,
                             int gutter_width, bool colors, const char *style) {
  print_line_gutter(out, 0, gutter_width, colors);

  /* Spaces to column */
  for (size_t i = 1; i < column; i++) {
    fputc(' ', out);
  }

  /* Carets */
  if (colors) {
    fprintf(out, "%s", style);
  }
  for (size_t i = 0; i < length && i < 50; i++) {
    fputc('^', out);
  }
  if (colors) {
    fprintf(out, "%s", jsbox_style_reset());
  }
}

static int count_digits(size_t n) {
  int count = 1;
  while (n >= 10) {
    count++;
    n /= 10;
  }
  return count;
}

/* ============================================================================
 * Reporting Implementation
 * ============================================================================
 */

void jsbox_reporter_emit(JSBox_Reporter *reporter,
                         const JSBox_Diagnostic *diag) {
  FILE *out = reporter->config.output;
  bool colors = reporter->config.colors;

  /* Calculate gutter width based on max line number */
  size_t max_line = diag->span.start.line + reporter->config.context_lines;
  int gutter_width = count_digits(max_line);
  if (gutter_width < 4)
    gutter_width = 4;

  /* === Header line === */
  /* error[E0001]: Message here */
  const char *level_style = get_level_style(diag->level);
  const char *level_name = jsbox_diag_level_name(diag->level);

  if (colors) {
    fprintf(out, "%s%s", level_style, level_name);
  } else {
    fprintf(out, "%s", level_name);
  }

  if (reporter->config.show_error_codes && diag->code > 0) {
    fprintf(out, "[%s]", jsbox_error_code_str(diag->code));
  }

  if (colors) {
    fprintf(out, ":%s %s\n", jsbox_style_reset(), diag->message);
  } else {
    fprintf(out, ": %s\n", diag->message);
  }

  /* === Location line === */
  /*   --> script.js:5:10 */
  if (!jsbox_span_is_empty(diag->span) && reporter->source) {
    if (colors) {
      fprintf(out, " %s-->%s %s:%zu:%zu\n", jsbox_style_location(),
              jsbox_style_reset(), reporter->source->filename,
              diag->span.start.line, diag->span.start.column);
    } else {
      fprintf(out, "  --> %s:%zu:%zu\n", reporter->source->filename,
              diag->span.start.line, diag->span.start.column);
    }

    /* === Empty gutter === */
    print_line_gutter(out, 0, gutter_width, colors);
    fprintf(out, "\n");

    /* === Source lines with context === */
    size_t start_line = diag->span.start.line;
    size_t end_line = diag->span.end.line;

    /* Context before */
    if (reporter->config.context_lines > 0 && start_line > 1) {
      size_t ctx_start = start_line > reporter->config.context_lines
                             ? start_line - reporter->config.context_lines
                             : 1;
      for (size_t ln = ctx_start; ln < start_line; ln++) {
        size_t line_len;
        const char *line =
            jsbox_source_file_get_line(reporter->source, ln, &line_len);
        if (line) {
          print_line_gutter(out, ln, gutter_width, colors);
          fwrite(line, 1, line_len, out);
          fprintf(out, "\n");
        }
      }
    }

    /* Error line(s) */
    for (size_t ln = start_line;
         ln <= end_line && ln <= reporter->source->line_count; ln++) {
      size_t line_len;
      const char *line =
          jsbox_source_file_get_line(reporter->source, ln, &line_len);
      if (line) {
        print_line_gutter(out, ln, gutter_width, colors);
        fwrite(line, 1, line_len, out);
        fprintf(out, "\n");

        /* Caret line */
        size_t caret_start = (ln == start_line) ? diag->span.start.column : 1;
        size_t caret_len;
        if (ln == end_line) {
          caret_len = diag->span.end.column - caret_start + 1;
        } else {
          caret_len = line_len - caret_start + 1;
        }
        if (caret_len < 1)
          caret_len = 1;

        print_caret_line(out, caret_start, caret_len, gutter_width, colors,
                         level_style);
        fprintf(out, "\n");
      }
    }

    /* Empty gutter */
    print_line_gutter(out, 0, gutter_width, colors);
    fprintf(out, "\n");
  }

  /* === Suggestion === */
  if (diag->suggestion) {
    if (colors) {
      fprintf(out, " %s=%s help: %s\n", jsbox_style_hint(), jsbox_style_reset(),
              diag->suggestion);
    } else {
      fprintf(out, "  = help: %s\n", diag->suggestion);
    }
  }

  /* === Related notes === */
  JSBox_Diagnostic *related = diag->related;
  while (related) {
    fprintf(out, "\n");
    jsbox_reporter_emit(reporter, related);
    related = related->related;
  }

  fprintf(out, "\n");
}

void jsbox_reporter_emit_all(JSBox_Reporter *reporter,
                             const JSBox_DiagnosticList *list) {
  for (size_t i = 0; i < list->count; i++) {
    jsbox_reporter_emit(reporter, list->items[i]);
  }
}

void jsbox_reporter_summary(JSBox_Reporter *reporter,
                            const JSBox_DiagnosticList *list) {
  FILE *out = reporter->config.output;
  bool colors = reporter->config.colors;

  if (list->error_count == 0 && list->warning_count == 0) {
    return;
  }

  if (colors) {
    if (list->error_count > 0) {
      fprintf(out, "%s%zu error%s%s", jsbox_style_error(), list->error_count,
              list->error_count == 1 ? "" : "s", jsbox_style_reset());
    }
    if (list->warning_count > 0) {
      if (list->error_count > 0) {
        fprintf(out, ", ");
      }
      fprintf(out, "%s%zu warning%s%s", jsbox_style_warning(),
              list->warning_count, list->warning_count == 1 ? "" : "s",
              jsbox_style_reset());
    }
  } else {
    if (list->error_count > 0) {
      fprintf(out, "%zu error%s", list->error_count,
              list->error_count == 1 ? "" : "s");
    }
    if (list->warning_count > 0) {
      if (list->error_count > 0) {
        fprintf(out, ", ");
      }
      fprintf(out, "%zu warning%s", list->warning_count,
              list->warning_count == 1 ? "" : "s");
    }
  }
  fprintf(out, " generated.\n");
}
