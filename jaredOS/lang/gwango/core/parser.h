/**
 * Gwango Language - Parser Header
 */

#ifndef GWANGO_PARSER_H
#define GWANGO_PARSER_H

#include "lexer.h"

/* AST node types */
typedef enum {
    NODE_PROGRAM,
    NODE_VAR_DECL,
    NODE_FN_DECL,
    NODE_RETURN,
    NODE_IF,
    NODE_LOOP,
    NODE_CALL,
    NODE_KCALL,     /* Kernel call @vga.print */
    NODE_BINARY,
    NODE_UNARY,
    NODE_NUMBER,
    NODE_STRING,
    NODE_IDENT,
    NODE_ASSIGN,
    NODE_ASM
} node_type_t;

/* Forward declare */
struct ast_node;

/* AST node structure */
typedef struct ast_node {
    node_type_t type;
    
    union {
        /* Number literal */
        int number;
        
        /* String/identifier */
        struct {
            const char *str;
            int len;
        } string;
        
        /* Binary op */
        struct {
            struct ast_node *left;
            struct ast_node *right;
            token_type_t op;
        } binary;
        
        /* Variable declaration */
        struct {
            const char *name;
            int name_len;
            struct ast_node *value;
        } var_decl;
        
        /* Function declaration */
        struct {
            const char *name;
            int name_len;
            const char **params;
            int *param_lens;
            int param_count;
            struct ast_node **body;
            int body_count;
        } fn_decl;
        
        /* Function/kernel call */
        struct {
            const char *name;
            int name_len;
            const char *module;  /* For kernel calls */
            int module_len;
            struct ast_node **args;
            int arg_count;
        } call;
        
        /* If statement */
        struct {
            struct ast_node *cond;
            struct ast_node **then_body;
            int then_count;
            struct ast_node **else_body;
            int else_count;
        } if_stmt;
        
        /* Loop */
        struct {
            const char *var;
            int var_len;
            struct ast_node *start;
            struct ast_node *end;
            struct ast_node **body;
            int body_count;
        } loop;
        
        /* Return */
        struct {
            struct ast_node *value;
        } ret;
        
        /* Inline asm */
        struct {
            const char *code;
            int len;
        } asm_block;
        
        /* Program */
        struct {
            struct ast_node **stmts;
            int stmt_count;
        } program;
    } data;
} ast_node_t;

/* Parser state */
typedef struct {
    lexer_t lexer;
    token_t current;
    token_t previous;
    bool had_error;
    const char *error_msg;
} parser_t;

/* Initialize parser */
void parser_init(parser_t *p, const char *source);

/* Parse program */
ast_node_t* parser_parse(parser_t *p);

/* Free AST */
void ast_free(ast_node_t *node);

#endif /* GWANGO_PARSER_H */
