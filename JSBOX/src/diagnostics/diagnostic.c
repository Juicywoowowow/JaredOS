/*
 * JSBOX - JavaScript Engine
 * 
 * Diagnostics: Error/Warning Implementation
 */

#include "diagnostic.h"
#include "../base/memory.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Diagnostic Creation
 * ============================================================================ */

JSBox_Diagnostic* jsbox_diagnostic_create(JSBox_DiagLevel level, JSBox_ErrorCode code,
                                           const char* message, JSBox_SourceSpan span) {
    JSBox_Diagnostic* diag = (JSBox_Diagnostic*)jsbox_calloc(1, sizeof(JSBox_Diagnostic));
    diag->level = level;
    diag->code = code;
    diag->message = jsbox_strdup(message);
    diag->span = span;
    diag->suggestion = NULL;
    diag->related = NULL;
    return diag;
}

void jsbox_diagnostic_add_suggestion(JSBox_Diagnostic* diag, const char* suggestion) {
    if (diag->suggestion) {
        jsbox_free(diag->suggestion);
    }
    diag->suggestion = jsbox_strdup(suggestion);
}

void jsbox_diagnostic_add_note(JSBox_Diagnostic* diag, const char* message, JSBox_SourceSpan span) {
    JSBox_Diagnostic* note = jsbox_diagnostic_create(JSBOX_DIAG_NOTE, 0, message, span);
    
    /* Add to end of related list */
    if (!diag->related) {
        diag->related = note;
    } else {
        JSBox_Diagnostic* last = diag->related;
        while (last->related) {
            last = last->related;
        }
        last->related = note;
    }
}

void jsbox_diagnostic_destroy(JSBox_Diagnostic* diag) {
    if (!diag) return;
    
    jsbox_free(diag->message);
    jsbox_free(diag->suggestion);
    
    /* Destroy related chain */
    JSBox_Diagnostic* related = diag->related;
    while (related) {
        JSBox_Diagnostic* next = related->related;
        jsbox_free(related->message);
        jsbox_free(related->suggestion);
        jsbox_free(related);
        related = next;
    }
    
    jsbox_free(diag);
}

/* ============================================================================
 * Diagnostic List
 * ============================================================================ */

#define DIAG_LIST_INITIAL_CAPACITY 8

JSBox_DiagnosticList* jsbox_diag_list_create(void) {
    JSBox_DiagnosticList* list = (JSBox_DiagnosticList*)jsbox_malloc(sizeof(JSBox_DiagnosticList));
    list->items = (JSBox_Diagnostic**)jsbox_malloc(sizeof(JSBox_Diagnostic*) * DIAG_LIST_INITIAL_CAPACITY);
    list->count = 0;
    list->capacity = DIAG_LIST_INITIAL_CAPACITY;
    list->error_count = 0;
    list->warning_count = 0;
    return list;
}

void jsbox_diag_list_destroy(JSBox_DiagnosticList* list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->count; i++) {
        jsbox_diagnostic_destroy(list->items[i]);
    }
    jsbox_free(list->items);
    jsbox_free(list);
}

void jsbox_diag_list_add(JSBox_DiagnosticList* list, JSBox_Diagnostic* diag) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = (JSBox_Diagnostic**)jsbox_realloc(list->items, 
                                                         sizeof(JSBox_Diagnostic*) * list->capacity);
    }
    
    list->items[list->count++] = diag;
    
    if (diag->level == JSBOX_DIAG_ERROR) {
        list->error_count++;
    } else if (diag->level == JSBOX_DIAG_WARNING) {
        list->warning_count++;
    }
}

void jsbox_diag_list_error(JSBox_DiagnosticList* list, JSBox_ErrorCode code,
                            const char* message, JSBox_SourceSpan span) {
    JSBox_Diagnostic* diag = jsbox_diagnostic_create(JSBOX_DIAG_ERROR, code, message, span);
    jsbox_diag_list_add(list, diag);
}

void jsbox_diag_list_warning(JSBox_DiagnosticList* list, JSBox_ErrorCode code,
                              const char* message, JSBox_SourceSpan span) {
    JSBox_Diagnostic* diag = jsbox_diagnostic_create(JSBOX_DIAG_WARNING, code, message, span);
    jsbox_diag_list_add(list, diag);
}

bool jsbox_diag_list_has_errors(const JSBox_DiagnosticList* list) {
    return list->error_count > 0;
}

void jsbox_diag_list_clear(JSBox_DiagnosticList* list) {
    for (size_t i = 0; i < list->count; i++) {
        jsbox_diagnostic_destroy(list->items[i]);
    }
    list->count = 0;
    list->error_count = 0;
    list->warning_count = 0;
}

/* ============================================================================
 * Error Code Helpers
 * ============================================================================ */

const char* jsbox_error_code_str(JSBox_ErrorCode code) {
    static char buf[8];
    snprintf(buf, sizeof(buf), "E%04d", code);
    return buf;
}

const char* jsbox_diag_level_name(JSBox_DiagLevel level) {
    switch (level) {
        case JSBOX_DIAG_ERROR:   return "error";
        case JSBOX_DIAG_WARNING: return "warning";
        case JSBOX_DIAG_NOTE:    return "note";
        case JSBOX_DIAG_HINT:    return "hint";
        default:                 return "unknown";
    }
}
