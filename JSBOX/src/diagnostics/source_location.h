/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: Source Location Tracking
 */

#ifndef JSBOX_SOURCE_LOCATION_H
#define JSBOX_SOURCE_LOCATION_H

#include <stddef.h>

/* ============================================================================
 * Source Location Types
 * ============================================================================ */

/* A single position in source code */
typedef struct {
    size_t line;        /* 1-indexed line number */
    size_t column;      /* 1-indexed column number */
    size_t offset;      /* 0-indexed byte offset from start */
} JSBox_SourcePos;

/* A span of source code (start..end) */
typedef struct {
    JSBox_SourcePos start;
    JSBox_SourcePos end;
} JSBox_SourceSpan;

/* Source file context */
typedef struct {
    const char* filename;    /* File name (may be "<stdin>" or "<eval>") */
    const char* source;      /* Full source text */
    size_t source_length;    /* Length of source */
    size_t* line_offsets;    /* Array of byte offsets for line starts */
    size_t line_count;       /* Number of lines */
} JSBox_SourceFile;

/* ============================================================================
 * Source File API
 * ============================================================================ */

/* Create source file context (scans for line offsets) */
JSBox_SourceFile* jsbox_source_file_create(const char* filename, const char* source);

/* Destroy source file */
void jsbox_source_file_destroy(JSBox_SourceFile* file);

/* Get line content (returns pointer into source, length in out_length) */
const char* jsbox_source_file_get_line(const JSBox_SourceFile* file, size_t line, size_t* out_length);

/* Get position from byte offset */
JSBox_SourcePos jsbox_source_file_pos_from_offset(const JSBox_SourceFile* file, size_t offset);

/* ============================================================================
 * Source Span API
 * ============================================================================ */

/* Create a span from line/column */
JSBox_SourceSpan jsbox_span_from_pos(size_t start_line, size_t start_col, 
                                      size_t end_line, size_t end_col);

/* Create a span from offsets */
JSBox_SourceSpan jsbox_span_from_offset(const JSBox_SourceFile* file,
                                         size_t start_offset, size_t end_offset);

/* Merge two spans */
JSBox_SourceSpan jsbox_span_merge(JSBox_SourceSpan a, JSBox_SourceSpan b);

/* Create an empty/invalid span */
JSBox_SourceSpan jsbox_span_empty(void);

/* Check if span is empty/invalid */
int jsbox_span_is_empty(JSBox_SourceSpan span);

#endif /* JSBOX_SOURCE_LOCATION_H */
