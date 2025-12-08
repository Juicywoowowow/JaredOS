/*
 * JSBOX - JavaScript Engine
 * 
 * Parsing: Lexer Implementation
 */

#include "lexer.h"
#include "../base/memory.h"
#include "../base/strings.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * Keyword Table
 * ============================================================================ */

typedef struct {
    const char* name;
    JSBox_TokenType type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"var",        JSBOX_TOK_VAR},
    {"let",        JSBOX_TOK_LET},
    {"const",      JSBOX_TOK_CONST},
    {"function",   JSBOX_TOK_FUNCTION},
    {"return",     JSBOX_TOK_RETURN},
    {"if",         JSBOX_TOK_IF},
    {"else",       JSBOX_TOK_ELSE},
    {"while",      JSBOX_TOK_WHILE},
    {"for",        JSBOX_TOK_FOR},
    {"do",         JSBOX_TOK_DO},
    {"break",      JSBOX_TOK_BREAK},
    {"continue",   JSBOX_TOK_CONTINUE},
    {"new",        JSBOX_TOK_NEW},
    {"this",       JSBOX_TOK_THIS},
    {"true",       JSBOX_TOK_TRUE},
    {"false",      JSBOX_TOK_FALSE},
    {"null",       JSBOX_TOK_NULL},
    {"undefined",  JSBOX_TOK_UNDEFINED},
    {"typeof",     JSBOX_TOK_TYPEOF},
    {"instanceof", JSBOX_TOK_INSTANCEOF},
    {"in",         JSBOX_TOK_IN},
    {"delete",     JSBOX_TOK_DELETE},
    {"void",       JSBOX_TOK_VOID},
    {"try",        JSBOX_TOK_TRY},
    {"catch",      JSBOX_TOK_CATCH},
    {"finally",    JSBOX_TOK_FINALLY},
    {"throw",      JSBOX_TOK_THROW},
    {"switch",     JSBOX_TOK_SWITCH},
    {"case",       JSBOX_TOK_CASE},
    {"default",    JSBOX_TOK_DEFAULT},
    {"class",      JSBOX_TOK_CLASS},
    {"extends",    JSBOX_TOK_EXTENDS},
    {"super",      JSBOX_TOK_SUPER},
    {"import",     JSBOX_TOK_IMPORT},
    {"export",     JSBOX_TOK_EXPORT},
    {NULL,         JSBOX_TOK_EOF}
};

static JSBox_TokenType lookup_keyword(const char* text, size_t length) {
    for (const KeywordEntry* kw = keywords; kw->name; kw++) {
        if (strlen(kw->name) == length && strncmp(kw->name, text, length) == 0) {
            return kw->type;
        }
    }
    return JSBOX_TOK_IDENTIFIER;
}

/* ============================================================================
 * Lexer Creation
 * ============================================================================ */

JSBox_Lexer* jsbox_lexer_create(const char* source, const char* filename) {
    JSBox_Lexer* lexer = (JSBox_Lexer*)jsbox_malloc(sizeof(JSBox_Lexer));
    
    lexer->source = source;
    lexer->current = source;
    lexer->token_start = source;
    
    lexer->line = 1;
    lexer->column = 1;
    lexer->offset = 0;
    
    lexer->source_file = jsbox_source_file_create(filename, source);
    lexer->diagnostics = jsbox_diag_list_create();
    
    return lexer;
}

void jsbox_lexer_destroy(JSBox_Lexer* lexer) {
    if (lexer) {
        jsbox_source_file_destroy(lexer->source_file);
        jsbox_diag_list_destroy(lexer->diagnostics);
        jsbox_free(lexer);
    }
}

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static bool is_at_end(JSBox_Lexer* lexer) {
    return *lexer->current == '\0';
}

static char peek(JSBox_Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(JSBox_Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static char advance(JSBox_Lexer* lexer) {
    char c = *lexer->current++;
    lexer->offset++;
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

static bool match(JSBox_Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    advance(lexer);
    return true;
}

static void start_token(JSBox_Lexer* lexer) {
    lexer->token_start = lexer->current;
    lexer->start_line = lexer->line;
    lexer->start_column = lexer->column;
    lexer->start_offset = lexer->offset;
}

static JSBox_Token make_token(JSBox_Lexer* lexer, JSBox_TokenType type) {
    JSBox_Token token;
    token.type = type;
    token.start = lexer->token_start;
    token.length = (size_t)(lexer->current - lexer->token_start);
    token.span.start.line = lexer->start_line;
    token.span.start.column = lexer->start_column;
    token.span.start.offset = lexer->start_offset;
    token.span.end.line = lexer->line;
    token.span.end.column = lexer->column;
    token.span.end.offset = lexer->offset;
    token.number_value = 0;
    token.string_value = NULL;
    return token;
}

static JSBox_Token error_token(JSBox_Lexer* lexer, JSBox_ErrorCode code, const char* message) {
    JSBox_Token token = make_token(lexer, JSBOX_TOK_ERROR);
    jsbox_diag_list_error(lexer->diagnostics, code, message, token.span);
    return token;
}

/* ============================================================================
 * Whitespace and Comments
 * ============================================================================ */

static void skip_whitespace(JSBox_Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    /* Single-line comment */
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    /* Multi-line comment */
                    advance(lexer);  /* / */
                    advance(lexer);  /* * */
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer);
                            advance(lexer);
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/* ============================================================================
 * Number Scanning
 * ============================================================================ */

static JSBox_Token scan_number(JSBox_Lexer* lexer) {
    bool is_hex = false;
    bool is_binary = false;
    bool is_octal = false;
    
    /* Check for hex/binary/octal prefix */
    if (peek(lexer) == '0') {
        char next = peek_next(lexer);
        if (next == 'x' || next == 'X') {
            is_hex = true;
            advance(lexer);
            advance(lexer);
        } else if (next == 'b' || next == 'B') {
            is_binary = true;
            advance(lexer);
            advance(lexer);
        } else if (next == 'o' || next == 'O') {
            is_octal = true;
            advance(lexer);
            advance(lexer);
        }
    }
    
    /* Scan digits */
    if (is_hex) {
        while (jsbox_is_digit(peek(lexer)) || 
               (peek(lexer) >= 'a' && peek(lexer) <= 'f') ||
               (peek(lexer) >= 'A' && peek(lexer) <= 'F')) {
            advance(lexer);
        }
    } else if (is_binary) {
        while (peek(lexer) == '0' || peek(lexer) == '1') {
            advance(lexer);
        }
    } else if (is_octal) {
        while (peek(lexer) >= '0' && peek(lexer) <= '7') {
            advance(lexer);
        }
    } else {
        while (jsbox_is_digit(peek(lexer))) {
            advance(lexer);
        }
        
        /* Decimal point */
        if (peek(lexer) == '.' && jsbox_is_digit(peek_next(lexer))) {
            advance(lexer);
            while (jsbox_is_digit(peek(lexer))) {
                advance(lexer);
            }
        }
        
        /* Exponent */
        if (peek(lexer) == 'e' || peek(lexer) == 'E') {
            advance(lexer);
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer);
            }
            while (jsbox_is_digit(peek(lexer))) {
                advance(lexer);
            }
        }
    }
    
    JSBox_Token token = make_token(lexer, JSBOX_TOK_NUMBER);
    
    /* Parse the number value */
    char* end;
    if (is_hex) {
        token.number_value = (double)strtoll(lexer->token_start, &end, 16);
    } else if (is_binary) {
        token.number_value = (double)strtoll(lexer->token_start + 2, &end, 2);
    } else if (is_octal) {
        token.number_value = (double)strtoll(lexer->token_start + 2, &end, 8);
    } else {
        token.number_value = strtod(lexer->token_start, &end);
    }
    
    return token;
}

/* ============================================================================
 * String Scanning
 * ============================================================================ */

static JSBox_Token scan_string(JSBox_Lexer* lexer, char quote) {
    JSBox_StringBuilder* sb = jsbox_sb_create();
    
    while (!is_at_end(lexer) && peek(lexer) != quote) {
        if (peek(lexer) == '\n') {
            jsbox_sb_destroy(sb);
            return error_token(lexer, JSBOX_ERR_UNTERMINATED_STRING, 
                              "Unterminated string literal");
        }
        
        if (peek(lexer) == '\\') {
            advance(lexer);
            if (is_at_end(lexer)) {
                jsbox_sb_destroy(sb);
                return error_token(lexer, JSBOX_ERR_UNTERMINATED_STRING,
                                  "Unterminated string literal");
            }
            
            char c = advance(lexer);
            switch (c) {
                case 'n':  jsbox_sb_append_char(sb, '\n'); break;
                case 'r':  jsbox_sb_append_char(sb, '\r'); break;
                case 't':  jsbox_sb_append_char(sb, '\t'); break;
                case '\\': jsbox_sb_append_char(sb, '\\'); break;
                case '\'': jsbox_sb_append_char(sb, '\''); break;
                case '"':  jsbox_sb_append_char(sb, '"'); break;
                case '0':  jsbox_sb_append_char(sb, '\0'); break;
                default:
                    /* Unknown escape - just include the character */
                    jsbox_sb_append_char(sb, c);
                    break;
            }
        } else {
            jsbox_sb_append_char(sb, advance(lexer));
        }
    }
    
    if (is_at_end(lexer)) {
        jsbox_sb_destroy(sb);
        return error_token(lexer, JSBOX_ERR_UNTERMINATED_STRING,
                          "Unterminated string literal");
    }
    
    /* Consume closing quote */
    advance(lexer);
    
    JSBox_Token token = make_token(lexer, JSBOX_TOK_STRING);
    token.string_value = jsbox_sb_to_string(sb);
    jsbox_sb_destroy(sb);
    
    return token;
}

/* ============================================================================
 * Identifier Scanning
 * ============================================================================ */

static JSBox_Token scan_identifier(JSBox_Lexer* lexer) {
    while (jsbox_is_ident_part(peek(lexer))) {
        advance(lexer);
    }
    
    size_t length = (size_t)(lexer->current - lexer->token_start);
    JSBox_TokenType type = lookup_keyword(lexer->token_start, length);
    
    return make_token(lexer, type);
}

/* ============================================================================
 * Main Scanning
 * ============================================================================ */

JSBox_Token jsbox_lexer_next(JSBox_Lexer* lexer) {
    skip_whitespace(lexer);
    start_token(lexer);
    
    if (is_at_end(lexer)) {
        return make_token(lexer, JSBOX_TOK_EOF);
    }
    
    char c = advance(lexer);
    
    /* Identifiers and keywords */
    if (jsbox_is_ident_start(c)) {
        return scan_identifier(lexer);
    }
    
    /* Numbers */
    if (jsbox_is_digit(c)) {
        return scan_number(lexer);
    }
    
    /* Strings */
    if (c == '"' || c == '\'') {
        return scan_string(lexer, c);
    }
    
    /* Operators and punctuation */
    switch (c) {
        case '(': return make_token(lexer, JSBOX_TOK_LPAREN);
        case ')': return make_token(lexer, JSBOX_TOK_RPAREN);
        case '{': return make_token(lexer, JSBOX_TOK_LBRACE);
        case '}': return make_token(lexer, JSBOX_TOK_RBRACE);
        case '[': return make_token(lexer, JSBOX_TOK_LBRACKET);
        case ']': return make_token(lexer, JSBOX_TOK_RBRACKET);
        case ',': return make_token(lexer, JSBOX_TOK_COMMA);
        case ';': return make_token(lexer, JSBOX_TOK_SEMICOLON);
        case ':': return make_token(lexer, JSBOX_TOK_COLON);
        case '~': return make_token(lexer, JSBOX_TOK_TILDE);
        
        case '.':
            if (match(lexer, '.') && match(lexer, '.')) {
                return make_token(lexer, JSBOX_TOK_DOTDOTDOT);
            }
            return make_token(lexer, JSBOX_TOK_DOT);
        
        case '+':
            if (match(lexer, '+')) return make_token(lexer, JSBOX_TOK_PLUS_PLUS);
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_PLUS_EQ);
            return make_token(lexer, JSBOX_TOK_PLUS);
        
        case '-':
            if (match(lexer, '-')) return make_token(lexer, JSBOX_TOK_MINUS_MINUS);
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_MINUS_EQ);
            return make_token(lexer, JSBOX_TOK_MINUS);
        
        case '*':
            if (match(lexer, '*')) return make_token(lexer, JSBOX_TOK_STAR_STAR);
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_STAR_EQ);
            return make_token(lexer, JSBOX_TOK_STAR);
        
        case '/':
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_SLASH_EQ);
            return make_token(lexer, JSBOX_TOK_SLASH);
        
        case '%':
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_PERCENT_EQ);
            return make_token(lexer, JSBOX_TOK_PERCENT);
        
        case '=':
            if (match(lexer, '>')) return make_token(lexer, JSBOX_TOK_ARROW);
            if (match(lexer, '=')) {
                if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_EQ_EQ_EQ);
                return make_token(lexer, JSBOX_TOK_EQ_EQ);
            }
            return make_token(lexer, JSBOX_TOK_EQ);
        
        case '!':
            if (match(lexer, '=')) {
                if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_BANG_EQ_EQ);
                return make_token(lexer, JSBOX_TOK_BANG_EQ);
            }
            return make_token(lexer, JSBOX_TOK_BANG);
        
        case '<':
            if (match(lexer, '<')) return make_token(lexer, JSBOX_TOK_LT_LT);
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_LT_EQ);
            return make_token(lexer, JSBOX_TOK_LT);
        
        case '>':
            if (match(lexer, '>')) {
                if (match(lexer, '>')) return make_token(lexer, JSBOX_TOK_GT_GT_GT);
                return make_token(lexer, JSBOX_TOK_GT_GT);
            }
            if (match(lexer, '=')) return make_token(lexer, JSBOX_TOK_GT_EQ);
            return make_token(lexer, JSBOX_TOK_GT);
        
        case '&':
            if (match(lexer, '&')) return make_token(lexer, JSBOX_TOK_AMP_AMP);
            return make_token(lexer, JSBOX_TOK_AMP);
        
        case '|':
            if (match(lexer, '|')) return make_token(lexer, JSBOX_TOK_PIPE_PIPE);
            return make_token(lexer, JSBOX_TOK_PIPE);
        
        case '^': return make_token(lexer, JSBOX_TOK_CARET);
        
        case '?':
            if (match(lexer, '?')) return make_token(lexer, JSBOX_TOK_QUESTION_QUESTION);
            if (match(lexer, '.')) return make_token(lexer, JSBOX_TOK_QUESTION_DOT);
            return make_token(lexer, JSBOX_TOK_QUESTION);
    }
    
    /* Unknown character */
    char msg[64];
    snprintf(msg, sizeof(msg), "Unexpected character '%c'", c);
    return error_token(lexer, JSBOX_ERR_UNEXPECTED_CHAR, msg);
}

JSBox_Token jsbox_lexer_peek(JSBox_Lexer* lexer) {
    /* Save state */
    const char* saved_current = lexer->current;
    const char* saved_start = lexer->token_start;
    size_t saved_line = lexer->line;
    size_t saved_column = lexer->column;
    size_t saved_offset = lexer->offset;
    
    JSBox_Token token = jsbox_lexer_next(lexer);
    
    /* Restore state */
    lexer->current = saved_current;
    lexer->token_start = saved_start;
    lexer->line = saved_line;
    lexer->column = saved_column;
    lexer->offset = saved_offset;
    
    return token;
}

JSBox_SourceFile* jsbox_lexer_source_file(JSBox_Lexer* lexer) {
    return lexer->source_file;
}

JSBox_DiagnosticList* jsbox_lexer_diagnostics(JSBox_Lexer* lexer) {
    return lexer->diagnostics;
}

bool jsbox_lexer_has_errors(const JSBox_Lexer* lexer) {
    return jsbox_diag_list_has_errors(lexer->diagnostics);
}
