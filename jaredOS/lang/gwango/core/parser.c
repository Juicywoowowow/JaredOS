/**
 * Gwango Language - Parser Implementation
 */

#include "parser.h"
#include "../../../kernel/lib/string.h"

/* Max nodes/params */
#define MAX_STMTS 64
#define MAX_ARGS 16
#define MAX_PARAMS 8
#define MAX_STMT_ARRAYS 16

/* Node allocation (simple bump allocator) */
static ast_node_t node_pool[512];
static int node_idx = 0;

static ast_node_t* alloc_node(void) {
    if (node_idx >= 512) return NULL;
    ast_node_t *n = &node_pool[node_idx++];
    memset(n, 0, sizeof(ast_node_t));
    return n;
}

/* Statement array pool - each block gets its own array */
static ast_node_t* stmt_pool[MAX_STMT_ARRAYS][MAX_STMTS];
static int stmt_pool_idx = 0;

static ast_node_t** alloc_stmt_array(void) {
    if (stmt_pool_idx >= MAX_STMT_ARRAYS) return NULL;
    return stmt_pool[stmt_pool_idx++];
}

/* Temporary storage for args and params */
static ast_node_t* arg_buf[MAX_ARGS];
static const char* param_names[MAX_PARAMS];
static int param_lens[MAX_PARAMS];

/* Forward declarations */
static ast_node_t* parse_expression(parser_t *p);
static ast_node_t* parse_statement(parser_t *p);

/* Advance token */
static void advance(parser_t *p) {
    p->previous = p->current;
    p->current = lexer_next(&p->lexer);
}

/* Check current token */
static bool check(parser_t *p, token_type_t type) {
    return p->current.type == type;
}

/* Match and advance */
static bool match(parser_t *p, token_type_t type) {
    if (!check(p, type)) return false;
    advance(p);
    return true;
}

/* Skip newlines */
static void skip_newlines(parser_t *p) {
    while (match(p, TOK_NEWLINE));
}

/* Error */
static void error(parser_t *p, const char *msg) {
    if (p->had_error) return;
    p->had_error = true;
    p->error_msg = msg;
}

/* Expect token */
static void expect(parser_t *p, token_type_t type, const char *msg) {
    if (!match(p, type)) {
        error(p, msg);
    }
}

/* Parse number */
static ast_node_t* parse_number(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_NUMBER;
    node->data.number = p->previous.value;
    return node;
}

/* Parse string */
static ast_node_t* parse_string(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_STRING;
    node->data.string.str = p->previous.start;
    node->data.string.len = p->previous.length;
    return node;
}

/* Parse identifier */
static ast_node_t* parse_ident(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_IDENT;
    node->data.string.str = p->previous.start;
    node->data.string.len = p->previous.length;
    return node;
}

/* Parse kernel call: @module.func args */
static ast_node_t* parse_kcall(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_KCALL;
    
    /* Module name */
    expect(p, TOK_IDENT, "Expected module name after @");
    node->data.call.module = p->previous.start;
    node->data.call.module_len = p->previous.length;
    
    /* Dot */
    expect(p, TOK_DOT, "Expected '.' after module");
    
    /* Function name */
    expect(p, TOK_IDENT, "Expected function name");
    node->data.call.name = p->previous.start;
    node->data.call.name_len = p->previous.length;
    
    /* Arguments (until newline or EOF) */
    int argc = 0;
    while (!check(p, TOK_NEWLINE) && !check(p, TOK_EOF) && argc < MAX_ARGS) {
        arg_buf[argc++] = parse_expression(p);
        if (!match(p, TOK_COMMA)) break;
    }
    
    node->data.call.args = (ast_node_t**)&arg_buf;
    node->data.call.arg_count = argc;
    return node;
}

/* Parse primary expression */
static ast_node_t* parse_primary(parser_t *p) {
    if (match(p, TOK_NUMBER)) return parse_number(p);
    if (match(p, TOK_STRING)) return parse_string(p);
    if (match(p, TOK_IDENT)) return parse_ident(p);
    if (match(p, TOK_AT)) return parse_kcall(p);
    if (match(p, TOK_LPAREN)) {
        ast_node_t *expr = parse_expression(p);
        expect(p, TOK_RPAREN, "Expected ')'");
        return expr;
    }
    error(p, "Expected expression");
    return NULL;
}

/* Parse call expression */
static ast_node_t* parse_call(parser_t *p) {
    ast_node_t *left = parse_primary(p);
    if (left && left->type == NODE_IDENT && match(p, TOK_LPAREN)) {
        ast_node_t *node = alloc_node();
        node->type = NODE_CALL;
        node->data.call.name = left->data.string.str;
        node->data.call.name_len = left->data.string.len;
        
        int argc = 0;
        if (!check(p, TOK_RPAREN)) {
            do {
                arg_buf[argc++] = parse_expression(p);
            } while (match(p, TOK_COMMA) && argc < MAX_ARGS);
        }
        expect(p, TOK_RPAREN, "Expected ')'");
        
        node->data.call.args = (ast_node_t**)&arg_buf;
        node->data.call.arg_count = argc;
        return node;
    }
    return left;
}

/* Parse unary */
static ast_node_t* parse_unary(parser_t *p) {
    if (match(p, TOK_MINUS)) {
        ast_node_t *node = alloc_node();
        node->type = NODE_UNARY;
        node->data.binary.op = TOK_MINUS;
        node->data.binary.right = parse_unary(p);
        return node;
    }
    return parse_call(p);
}

/* Parse factor (*, /) */
static ast_node_t* parse_factor(parser_t *p) {
    ast_node_t *left = parse_unary(p);
    while (match(p, TOK_STAR) || match(p, TOK_SLASH)) {
        token_type_t op = p->previous.type;
        ast_node_t *right = parse_unary(p);
        ast_node_t *node = alloc_node();
        node->type = NODE_BINARY;
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        left = node;
    }
    return left;
}

/* Parse term (+, -) */
static ast_node_t* parse_term(parser_t *p) {
    ast_node_t *left = parse_factor(p);
    while (match(p, TOK_PLUS) || match(p, TOK_MINUS)) {
        token_type_t op = p->previous.type;
        ast_node_t *right = parse_factor(p);
        ast_node_t *node = alloc_node();
        node->type = NODE_BINARY;
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        left = node;
    }
    return left;
}

/* Parse comparison */
static ast_node_t* parse_comparison(parser_t *p) {
    ast_node_t *left = parse_term(p);
    while (match(p, TOK_LT) || match(p, TOK_GT) || 
           match(p, TOK_LE) || match(p, TOK_GE) ||
           match(p, TOK_EQEQ) || match(p, TOK_NE)) {
        token_type_t op = p->previous.type;
        ast_node_t *right = parse_term(p);
        ast_node_t *node = alloc_node();
        node->type = NODE_BINARY;
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        left = node;
    }
    return left;
}

/* Parse expression */
static ast_node_t* parse_expression(parser_t *p) {
    return parse_comparison(p);
}

/* Parse var declaration */
static ast_node_t* parse_var(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_VAR_DECL;
    
    expect(p, TOK_IDENT, "Expected variable name");
    node->data.var_decl.name = p->previous.start;
    node->data.var_decl.name_len = p->previous.length;
    
    expect(p, TOK_EQ, "Expected '='");
    node->data.var_decl.value = parse_expression(p);
    return node;
}

/* Parse return */
static ast_node_t* parse_return(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_RETURN;
    node->data.ret.value = parse_expression(p);
    return node;
}

/* Parse block until 'end' or 'else' */
static int parse_block(parser_t *p, ast_node_t **stmts) {
    int count = 0;
    skip_newlines(p);
    while (!check(p, TOK_END) && !check(p, TOK_ELSE) && 
           !check(p, TOK_EOF) && count < MAX_STMTS) {
        stmts[count] = parse_statement(p);
        if (stmts[count]) count++;
        skip_newlines(p);
    }
    return count;
}

/* Parse if */
static ast_node_t* parse_if(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_IF;
    node->data.if_stmt.cond = parse_expression(p);
    
    ast_node_t **then_stmts = alloc_stmt_array();
    node->data.if_stmt.then_count = parse_block(p, then_stmts);
    node->data.if_stmt.then_body = then_stmts;
    
    if (match(p, TOK_ELSE)) {
        skip_newlines(p);
        ast_node_t **else_stmts = alloc_stmt_array();
        node->data.if_stmt.else_count = parse_block(p, else_stmts);
        node->data.if_stmt.else_body = else_stmts;
    }
    
    expect(p, TOK_END, "Expected 'end'");
    return node;
}

/* Parse loop */
static ast_node_t* parse_loop(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_LOOP;
    
    expect(p, TOK_IDENT, "Expected loop variable");
    node->data.loop.var = p->previous.start;
    node->data.loop.var_len = p->previous.length;
    
    expect(p, TOK_EQ, "Expected '='");
    node->data.loop.start = parse_expression(p);
    
    expect(p, TOK_TO, "Expected 'to'");
    node->data.loop.end = parse_expression(p);
    
    ast_node_t **loop_stmts = alloc_stmt_array();
    node->data.loop.body_count = parse_block(p, loop_stmts);
    node->data.loop.body = loop_stmts;
    
    expect(p, TOK_END, "Expected 'end'");
    return node;
}

/* Parse function */
static ast_node_t* parse_fn(parser_t *p) {
    ast_node_t *node = alloc_node();
    node->type = NODE_FN_DECL;
    
    expect(p, TOK_IDENT, "Expected function name");
    node->data.fn_decl.name = p->previous.start;
    node->data.fn_decl.name_len = p->previous.length;
    
    expect(p, TOK_LPAREN, "Expected '('");
    
    int param_count = 0;
    if (!check(p, TOK_RPAREN)) {
        do {
            expect(p, TOK_IDENT, "Expected parameter name");
            param_names[param_count] = p->previous.start;
            param_lens[param_count] = p->previous.length;
            param_count++;
        } while (match(p, TOK_COMMA) && param_count < MAX_PARAMS);
    }
    expect(p, TOK_RPAREN, "Expected ')'");
    
    node->data.fn_decl.params = param_names;
    node->data.fn_decl.param_lens = param_lens;
    node->data.fn_decl.param_count = param_count;
    
    ast_node_t **fn_stmts = alloc_stmt_array();
    node->data.fn_decl.body_count = parse_block(p, fn_stmts);
    node->data.fn_decl.body = fn_stmts;
    
    expect(p, TOK_END, "Expected 'end'");
    return node;
}

/* Parse statement */
static ast_node_t* parse_statement(parser_t *p) {
    skip_newlines(p);
    
    if (match(p, TOK_VAR)) return parse_var(p);
    if (match(p, TOK_RET)) return parse_return(p);
    if (match(p, TOK_IF)) return parse_if(p);
    if (match(p, TOK_LOOP)) return parse_loop(p);
    if (match(p, TOK_FN)) return parse_fn(p);
    if (match(p, TOK_AT)) return parse_kcall(p);
    
    /* Expression statement or assignment */
    ast_node_t *expr = parse_expression(p);
    if (expr && expr->type == NODE_IDENT && match(p, TOK_EQ)) {
        ast_node_t *node = alloc_node();
        node->type = NODE_ASSIGN;
        node->data.var_decl.name = expr->data.string.str;
        node->data.var_decl.name_len = expr->data.string.len;
        node->data.var_decl.value = parse_expression(p);
        return node;
    }
    return expr;
}

/* Initialize parser */
void parser_init(parser_t *p, const char *source) {
    lexer_init(&p->lexer, source);
    p->had_error = false;
    p->error_msg = NULL;
    node_idx = 0;
    stmt_pool_idx = 0;
    advance(p);
}

/* Parse program */
ast_node_t* parser_parse(parser_t *p) {
    ast_node_t *program = alloc_node();
    program->type = NODE_PROGRAM;
    
    ast_node_t **prog_stmts = alloc_stmt_array();
    int count = 0;
    skip_newlines(p);
    while (!check(p, TOK_EOF) && count < MAX_STMTS) {
        ast_node_t *stmt = parse_statement(p);
        if (stmt) prog_stmts[count++] = stmt;
        skip_newlines(p);
        if (p->had_error) break;
    }
    
    program->data.program.stmts = prog_stmts;
    program->data.program.stmt_count = count;
    return program;
}

/* Free AST (no-op with pool allocator) */
void ast_free(ast_node_t *node) {
    (void)node;
    node_idx = 0;
}
