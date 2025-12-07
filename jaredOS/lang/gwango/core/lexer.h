/**
 * Gwango Language - Lexer Header
 */

#ifndef GWANGO_LEXER_H
#define GWANGO_LEXER_H

#include "../../../kernel/types.h"

/* Token types */
typedef enum {
    TOK_EOF = 0,
    TOK_NEWLINE,
    
    /* Literals */
    TOK_NUMBER,
    TOK_STRING,
    TOK_IDENT,
    
    /* Keywords */
    TOK_VAR,
    TOK_FN,
    TOK_RET,
    TOK_IF,
    TOK_ELSE,
    TOK_END,
    TOK_LOOP,
    TOK_TO,
    TOK_ASM,
    
    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_EQ,
    TOK_EQEQ,
    TOK_NE,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,
    
    /* Delimiters */
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_AT,
    TOK_DOT,
    
    /* Special */
    TOK_ERROR
} token_type_t;

/* Token structure */
typedef struct {
    token_type_t type;
    const char *start;
    int length;
    int line;
    int value;  /* For numbers */
} token_t;

/* Lexer state */
typedef struct {
    const char *source;
    const char *current;
    int line;
} lexer_t;

/* Initialize lexer */
void lexer_init(lexer_t *lex, const char *source);

/* Get next token */
token_t lexer_next(lexer_t *lex);

/* Peek at next token */
token_t lexer_peek(lexer_t *lex);

/* Token name for debugging */
const char* token_name(token_type_t type);

#endif /* GWANGO_LEXER_H */
