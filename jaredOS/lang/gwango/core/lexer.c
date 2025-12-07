/**
 * Gwango Language - Lexer Implementation
 */

#include "lexer.h"
#include "../../../kernel/lib/string.h"

/* Check if character is digit */
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/* Check if character is alpha */
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/* Check if at end */
static bool is_at_end(lexer_t *lex) {
    return *lex->current == '\0';
}

/* Advance and return current char */
static char advance(lexer_t *lex) {
    return *lex->current++;
}

/* Peek current char */
static char peek(lexer_t *lex) {
    return *lex->current;
}

/* Peek next char */
static char peek_next(lexer_t *lex) {
    if (is_at_end(lex)) return '\0';
    return lex->current[1];
}

/* Match current char */
static bool match(lexer_t *lex, char expected) {
    if (is_at_end(lex)) return false;
    if (*lex->current != expected) return false;
    lex->current++;
    return true;
}

/* Skip whitespace and comments */
static void skip_whitespace(lexer_t *lex) {
    for (;;) {
        char c = peek(lex);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance(lex);
                break;
            case ';':  /* Comment */
                while (peek(lex) != '\n' && !is_at_end(lex)) {
                    advance(lex);
                }
                break;
            default:
                return;
        }
    }
}

/* Make token */
static token_t make_token(lexer_t *lex, token_type_t type, const char *start) {
    token_t tok;
    tok.type = type;
    tok.start = start;
    tok.length = (int)(lex->current - start);
    tok.line = lex->line;
    tok.value = 0;
    return tok;
}

/* Make error token */
static token_t error_token(lexer_t *lex, const char *msg) {
    token_t tok;
    tok.type = TOK_ERROR;
    tok.start = msg;
    tok.length = strlen(msg);
    tok.line = lex->line;
    tok.value = 0;
    return tok;
}

/* Check keyword */
static token_type_t check_keyword(const char *start, int len, 
                                   const char *rest, int rest_len,
                                   token_type_t type) {
    if (len == rest_len + 1) {
        bool match = true;
        for (int i = 0; i < rest_len; i++) {
            if (start[1 + i] != rest[i]) {
                match = false;
                break;
            }
        }
        if (match) return type;
    }
    return TOK_IDENT;
}

/* Identify keyword or identifier */
static token_type_t ident_type(const char *start, int len) {
    switch (start[0]) {
        case 'a': return check_keyword(start, len, "sm", 2, TOK_ASM);
        case 'e':
            if (len > 1) {
                switch (start[1]) {
                    case 'l': return check_keyword(start, len, "lse", 3, TOK_ELSE);
                    case 'n': return check_keyword(start, len, "nd", 2, TOK_END);
                }
            }
            break;
        case 'f': return check_keyword(start, len, "n", 1, TOK_FN);
        case 'i': return check_keyword(start, len, "f", 1, TOK_IF);
        case 'l': return check_keyword(start, len, "oop", 3, TOK_LOOP);
        case 'r': return check_keyword(start, len, "et", 2, TOK_RET);
        case 't': return check_keyword(start, len, "o", 1, TOK_TO);
        case 'v': return check_keyword(start, len, "ar", 2, TOK_VAR);
    }
    return TOK_IDENT;
}

/* Scan identifier */
static token_t scan_ident(lexer_t *lex, const char *start) {
    while (is_alpha(peek(lex)) || is_digit(peek(lex))) {
        advance(lex);
    }
    int len = (int)(lex->current - start);
    token_t tok = make_token(lex, ident_type(start, len), start);
    return tok;
}

/* Scan number */
static token_t scan_number(lexer_t *lex, const char *start) {
    while (is_digit(peek(lex))) {
        advance(lex);
    }
    token_t tok = make_token(lex, TOK_NUMBER, start);
    
    /* Parse value */
    int val = 0;
    for (const char *p = start; p < lex->current; p++) {
        val = val * 10 + (*p - '0');
    }
    tok.value = val;
    return tok;
}

/* Scan string */
static token_t scan_string(lexer_t *lex) {
    const char *start = lex->current;
    while (peek(lex) != '"' && !is_at_end(lex)) {
        if (peek(lex) == '\n') lex->line++;
        advance(lex);
    }
    if (is_at_end(lex)) {
        return error_token(lex, "Unterminated string");
    }
    token_t tok = make_token(lex, TOK_STRING, start);
    advance(lex);  /* Closing " */
    return tok;
}

/* Initialize lexer */
void lexer_init(lexer_t *lex, const char *source) {
    lex->source = source;
    lex->current = source;
    lex->line = 1;
}

/* Get next token */
token_t lexer_next(lexer_t *lex) {
    skip_whitespace(lex);
    
    if (is_at_end(lex)) {
        return make_token(lex, TOK_EOF, lex->current);
    }
    
    const char *start = lex->current;
    char c = advance(lex);
    
    /* Identifiers and keywords */
    if (is_alpha(c)) {
        return scan_ident(lex, start);
    }
    
    /* Numbers */
    if (is_digit(c)) {
        return scan_number(lex, start);
    }
    
    switch (c) {
        case '\n':
            lex->line++;
            return make_token(lex, TOK_NEWLINE, start);
        case '"':
            return scan_string(lex);
        case '(':
            return make_token(lex, TOK_LPAREN, start);
        case ')':
            return make_token(lex, TOK_RPAREN, start);
        case ',':
            return make_token(lex, TOK_COMMA, start);
        case '@':
            return make_token(lex, TOK_AT, start);
        case '.':
            return make_token(lex, TOK_DOT, start);
        case '+':
            return make_token(lex, TOK_PLUS, start);
        case '-':
            return make_token(lex, TOK_MINUS, start);
        case '*':
            return make_token(lex, TOK_STAR, start);
        case '/':
            return make_token(lex, TOK_SLASH, start);
        case '=':
            return make_token(lex, match(lex, '=') ? TOK_EQEQ : TOK_EQ, start);
        case '!':
            return match(lex, '=') ? make_token(lex, TOK_NE, start) 
                                   : error_token(lex, "Expected '='");
        case '<':
            return make_token(lex, match(lex, '=') ? TOK_LE : TOK_LT, start);
        case '>':
            return make_token(lex, match(lex, '=') ? TOK_GE : TOK_GT, start);
    }
    
    return error_token(lex, "Unexpected character");
}

/* Peek at next token */
token_t lexer_peek(lexer_t *lex) {
    lexer_t saved = *lex;
    token_t tok = lexer_next(lex);
    *lex = saved;
    return tok;
}

/* Token name for debugging */
const char* token_name(token_type_t type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_NEWLINE: return "NEWLINE";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_IDENT: return "IDENT";
        case TOK_VAR: return "VAR";
        case TOK_FN: return "FN";
        case TOK_RET: return "RET";
        case TOK_IF: return "IF";
        case TOK_ELSE: return "ELSE";
        case TOK_END: return "END";
        case TOK_LOOP: return "LOOP";
        case TOK_TO: return "TO";
        case TOK_ASM: return "ASM";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_STAR: return "STAR";
        case TOK_SLASH: return "SLASH";
        case TOK_EQ: return "EQ";
        case TOK_EQEQ: return "EQEQ";
        case TOK_NE: return "NE";
        case TOK_LT: return "LT";
        case TOK_GT: return "GT";
        case TOK_LE: return "LE";
        case TOK_GE: return "GE";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_COMMA: return "COMMA";
        case TOK_AT: return "AT";
        case TOK_DOT: return "DOT";
        case TOK_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}
