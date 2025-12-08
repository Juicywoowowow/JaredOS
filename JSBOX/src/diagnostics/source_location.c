/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: Source Location Implementation
 */

#include "source_location.h"
#include "../base/memory.h"
#include "../base/vector.h"
#include <string.h>

/* Vector of size_t for line offsets */
JSBOX_VEC_DEFINE(size_t, SizeTVec)

JSBox_SourceFile* jsbox_source_file_create(const char* filename, const char* source) {
    JSBox_SourceFile* file = (JSBox_SourceFile*)jsbox_malloc(sizeof(JSBox_SourceFile));
    
    file->filename = jsbox_strdup(filename);
    file->source = source;  /* Note: we don't copy, caller owns */
    file->source_length = strlen(source);
    
    /* Scan for line offsets */
    SizeTVec* offsets = SizeTVec_create();
    SizeTVec_push(offsets, 0);  /* First line starts at 0 */
    
    for (size_t i = 0; i < file->source_length; i++) {
        if (source[i] == '\n') {
            SizeTVec_push(offsets, i + 1);
        }
    }
    
    file->line_count = SizeTVec_length(offsets);
    file->line_offsets = (size_t*)jsbox_malloc(sizeof(size_t) * file->line_count);
    memcpy(file->line_offsets, offsets->data, sizeof(size_t) * file->line_count);
    
    SizeTVec_destroy(offsets);
    
    return file;
}

void jsbox_source_file_destroy(JSBox_SourceFile* file) {
    if (file) {
        jsbox_free((void*)file->filename);
        jsbox_free(file->line_offsets);
        jsbox_free(file);
    }
}

const char* jsbox_source_file_get_line(const JSBox_SourceFile* file, size_t line, size_t* out_length) {
    if (line == 0 || line > file->line_count) {
        *out_length = 0;
        return NULL;
    }
    
    size_t start = file->line_offsets[line - 1];
    size_t end;
    
    if (line < file->line_count) {
        end = file->line_offsets[line] - 1;  /* Exclude newline */
    } else {
        end = file->source_length;
    }
    
    /* Skip trailing \r if present (Windows line endings) */
    if (end > start && file->source[end - 1] == '\r') {
        end--;
    }
    
    *out_length = end - start;
    return file->source + start;
}

JSBox_SourcePos jsbox_source_file_pos_from_offset(const JSBox_SourceFile* file, size_t offset) {
    JSBox_SourcePos pos = {1, 1, offset};
    
    /* Binary search for line */
    size_t low = 0;
    size_t high = file->line_count;
    
    while (low < high) {
        size_t mid = (low + high) / 2;
        if (file->line_offsets[mid] <= offset) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    
    pos.line = low;  /* 1-indexed */
    if (pos.line > 0) {
        pos.column = offset - file->line_offsets[pos.line - 1] + 1;
    }
    
    return pos;
}

JSBox_SourceSpan jsbox_span_from_pos(size_t start_line, size_t start_col, 
                                      size_t end_line, size_t end_col) {
    JSBox_SourceSpan span = {
        .start = {start_line, start_col, 0},
        .end = {end_line, end_col, 0}
    };
    return span;
}

JSBox_SourceSpan jsbox_span_from_offset(const JSBox_SourceFile* file,
                                         size_t start_offset, size_t end_offset) {
    JSBox_SourceSpan span = {
        .start = jsbox_source_file_pos_from_offset(file, start_offset),
        .end = jsbox_source_file_pos_from_offset(file, end_offset)
    };
    return span;
}

JSBox_SourceSpan jsbox_span_merge(JSBox_SourceSpan a, JSBox_SourceSpan b) {
    JSBox_SourceSpan merged;
    
    /* Pick earlier start */
    if (a.start.offset <= b.start.offset) {
        merged.start = a.start;
    } else {
        merged.start = b.start;
    }
    
    /* Pick later end */
    if (a.end.offset >= b.end.offset) {
        merged.end = a.end;
    } else {
        merged.end = b.end;
    }
    
    return merged;
}

JSBox_SourceSpan jsbox_span_empty(void) {
    JSBox_SourceSpan span = {{0, 0, 0}, {0, 0, 0}};
    return span;
}

int jsbox_span_is_empty(JSBox_SourceSpan span) {
    return span.start.line == 0;
}
