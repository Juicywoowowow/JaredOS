/*
 * JSBOX - JavaScript Engine
 * 
 * Parsing: Token Definitions
 */

#ifndef JSBOX_TOKENS_H
#define JSBOX_TOKENS_H

#include "../diagnostics/source_location.h"
#include <stddef.h>

/* ============================================================================
 * Token Types
 * ============================================================================ */

typedef enum {
    /* Special */
    JSBOX_TOK_EOF = 0,
    JSBOX_TOK_ERROR,
    
    /* Literals */
    JSBOX_TOK_NUMBER,        /* 42, 3.14, 0xFF */
    JSBOX_TOK_STRING,        /* "hello", 'world' */
    JSBOX_TOK_IDENTIFIER,    /* foo, bar, _test */
    
    /* Keywords */
    JSBOX_TOK_VAR,           /* var */
    JSBOX_TOK_LET,           /* let */
    JSBOX_TOK_CONST,         /* const */
    JSBOX_TOK_FUNCTION,      /* function */
    JSBOX_TOK_RETURN,        /* return */
    JSBOX_TOK_IF,            /* if */
    JSBOX_TOK_ELSE,          /* else */
    JSBOX_TOK_WHILE,         /* while */
    JSBOX_TOK_FOR,           /* for */
    JSBOX_TOK_DO,            /* do */
    JSBOX_TOK_BREAK,         /* break */
    JSBOX_TOK_CONTINUE,      /* continue */
    JSBOX_TOK_NEW,           /* new */
    JSBOX_TOK_THIS,          /* this */
    JSBOX_TOK_TRUE,          /* true */
    JSBOX_TOK_FALSE,         /* false */
    JSBOX_TOK_NULL,          /* null */
    JSBOX_TOK_UNDEFINED,     /* undefined */
    JSBOX_TOK_TYPEOF,        /* typeof */
    JSBOX_TOK_INSTANCEOF,    /* instanceof */
    JSBOX_TOK_IN,            /* in */
    JSBOX_TOK_DELETE,        /* delete */
    JSBOX_TOK_VOID,          /* void */
    JSBOX_TOK_TRY,           /* try */
    JSBOX_TOK_CATCH,         /* catch */
    JSBOX_TOK_FINALLY,       /* finally */
    JSBOX_TOK_THROW,         /* throw */
    JSBOX_TOK_SWITCH,        /* switch */
    JSBOX_TOK_CASE,          /* case */
    JSBOX_TOK_DEFAULT,       /* default */
    JSBOX_TOK_CLASS,         /* class (reserved) */
    JSBOX_TOK_EXTENDS,       /* extends (reserved) */
    JSBOX_TOK_SUPER,         /* super (reserved) */
    JSBOX_TOK_IMPORT,        /* import (reserved) */
    JSBOX_TOK_EXPORT,        /* export (reserved) */
    
    /* Operators */
    JSBOX_TOK_PLUS,          /* + */
    JSBOX_TOK_MINUS,         /* - */
    JSBOX_TOK_STAR,          /* * */
    JSBOX_TOK_SLASH,         /* / */
    JSBOX_TOK_PERCENT,       /* % */
    JSBOX_TOK_STAR_STAR,     /* ** */
    JSBOX_TOK_PLUS_PLUS,     /* ++ */
    JSBOX_TOK_MINUS_MINUS,   /* -- */
    
    JSBOX_TOK_EQ,            /* = */
    JSBOX_TOK_PLUS_EQ,       /* += */
    JSBOX_TOK_MINUS_EQ,      /* -= */
    JSBOX_TOK_STAR_EQ,       /* *= */
    JSBOX_TOK_SLASH_EQ,      /* /= */
    JSBOX_TOK_PERCENT_EQ,    /* %= */
    
    JSBOX_TOK_EQ_EQ,         /* == */
    JSBOX_TOK_EQ_EQ_EQ,      /* === */
    JSBOX_TOK_BANG_EQ,       /* != */
    JSBOX_TOK_BANG_EQ_EQ,    /* !== */
    JSBOX_TOK_LT,            /* < */
    JSBOX_TOK_GT,            /* > */
    JSBOX_TOK_LT_EQ,         /* <= */
    JSBOX_TOK_GT_EQ,         /* >= */
    
    JSBOX_TOK_AMP,           /* & */
    JSBOX_TOK_PIPE,          /* | */
    JSBOX_TOK_CARET,         /* ^ */
    JSBOX_TOK_TILDE,         /* ~ */
    JSBOX_TOK_LT_LT,         /* << */
    JSBOX_TOK_GT_GT,         /* >> */
    JSBOX_TOK_GT_GT_GT,      /* >>> */
    
    JSBOX_TOK_AMP_AMP,       /* && */
    JSBOX_TOK_PIPE_PIPE,     /* || */
    JSBOX_TOK_BANG,          /* ! */
    JSBOX_TOK_QUESTION,      /* ? */
    JSBOX_TOK_QUESTION_QUESTION, /* ?? */
    JSBOX_TOK_QUESTION_DOT,  /* ?. */
    
    /* Punctuation */
    JSBOX_TOK_LPAREN,        /* ( */
    JSBOX_TOK_RPAREN,        /* ) */
    JSBOX_TOK_LBRACE,        /* { */
    JSBOX_TOK_RBRACE,        /* } */
    JSBOX_TOK_LBRACKET,      /* [ */
    JSBOX_TOK_RBRACKET,      /* ] */
    JSBOX_TOK_COMMA,         /* , */
    JSBOX_TOK_DOT,           /* . */
    JSBOX_TOK_SEMICOLON,     /* ; */
    JSBOX_TOK_COLON,         /* : */
    
    /* Arrow */
    JSBOX_TOK_ARROW,         /* => */
    JSBOX_TOK_DOTDOTDOT,     /* ... */
    
    JSBOX_TOK_COUNT
} JSBox_TokenType;

/* ============================================================================
 * Token Structure
 * ============================================================================ */

typedef struct {
    JSBox_TokenType type;
    const char* start;       /* Pointer into source */
    size_t length;           /* Length of token text */
    JSBox_SourceSpan span;   /* Source location */
    
    /* For number literals */
    double number_value;
    
    /* For string literals (unescaped content, heap allocated) */
    char* string_value;
} JSBox_Token;

/* ============================================================================
 * Token Helpers
 * ============================================================================ */

/* Get token type name for debugging */
const char* jsbox_token_type_name(JSBox_TokenType type);

/* Get token text (copies to buffer) */
void jsbox_token_text(const JSBox_Token* token, char* buffer, size_t buffer_size);

/* Free token resources (string_value) */
void jsbox_token_free(JSBox_Token* token);

#endif /* JSBOX_TOKENS_H */
