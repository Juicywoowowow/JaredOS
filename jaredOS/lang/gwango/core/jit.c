/**
 * Gwango Language - JIT Compiler Implementation
 * 
 * Compiles AST directly to x86 machine code
 */

#include "jit.h"
#include "../../../kernel/lib/string.h"
#include "../../../kernel/drivers/vga.h"
#include "../../../kernel/lib/printf.h"

/* Variables storage */
#define MAX_VARS 64
static struct {
    const char *name;
    int name_len;
    int value;
} variables[MAX_VARS];
static int var_count = 0;

/* Find variable */
static int find_var(const char *name, int len) {
    for (int i = 0; i < var_count; i++) {
        if (variables[i].name_len == len) {
            bool match = true;
            for (int j = 0; j < len; j++) {
                if (variables[i].name[j] != name[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }
    }
    return -1;
}

/* Set variable */
static void set_var(const char *name, int len, int value) {
    int idx = find_var(name, len);
    if (idx < 0) {
        idx = var_count++;
        variables[idx].name = name;
        variables[idx].name_len = len;
    }
    variables[idx].value = value;
}

/* Get variable */
static int get_var(const char *name, int len) {
    int idx = find_var(name, len);
    if (idx >= 0) return variables[idx].value;
    return 0;
}

/* Kernel call handlers */
static void kcall_vga_print_num(int n) {
    kprintf("%d", n);
}

static void kcall_vga_print_str(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        vga_putchar(s[i]);
    }
}

static void kcall_vga_clear(void) {
    vga_clear();
}

static void kcall_vga_newline(void) {
    kprintf("\n");
}

/* Evaluate expression (interpreter for now) */
static int eval_expr(ast_node_t *node);

static int eval_expr(ast_node_t *node) {
    if (!node) return 0;
    
    switch (node->type) {
        case NODE_NUMBER:
            return node->data.number;
            
        case NODE_IDENT:
            return get_var(node->data.string.str, node->data.string.len);
            
        case NODE_BINARY: {
            int left = eval_expr(node->data.binary.left);
            int right = eval_expr(node->data.binary.right);
            switch (node->data.binary.op) {
                case TOK_PLUS: return left + right;
                case TOK_MINUS: return left - right;
                case TOK_STAR: return left * right;
                case TOK_SLASH: return right ? left / right : 0;
                case TOK_LT: return left < right;
                case TOK_GT: return left > right;
                case TOK_LE: return left <= right;
                case TOK_GE: return left >= right;
                case TOK_EQEQ: return left == right;
                case TOK_NE: return left != right;
                default: return 0;
            }
        }
        
        case NODE_UNARY:
            return -eval_expr(node->data.binary.right);
            
        default:
            return 0;
    }
}

/* Execute statement */
static void exec_stmt(ast_node_t *node);

static void exec_stmt(ast_node_t *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_VAR_DECL: {
            int val = eval_expr(node->data.var_decl.value);
            set_var(node->data.var_decl.name, node->data.var_decl.name_len, val);
            break;
        }
        
        case NODE_ASSIGN: {
            int val = eval_expr(node->data.var_decl.value);
            set_var(node->data.var_decl.name, node->data.var_decl.name_len, val);
            break;
        }
        
        case NODE_KCALL: {
            const char *mod = node->data.call.module;
            int mod_len = node->data.call.module_len;
            const char *fn = node->data.call.name;
            int fn_len = node->data.call.name_len;
            
            /* @vga module */
            if (mod_len == 3 && mod[0] == 'v' && mod[1] == 'g' && mod[2] == 'a') {
                if (fn_len == 5 && strncmp(fn, "print", 5) == 0) {
                    if (node->data.call.arg_count > 0) {
                        ast_node_t *arg = node->data.call.args[0];
                        if (arg->type == NODE_STRING) {
                            kcall_vga_print_str(arg->data.string.str, arg->data.string.len);
                        } else {
                            int val = eval_expr(arg);
                            kcall_vga_print_num(val);
                        }
                    }
                } else if (fn_len == 5 && strncmp(fn, "clear", 5) == 0) {
                    kcall_vga_clear();
                } else if (fn_len == 7 && strncmp(fn, "newline", 7) == 0) {
                    kcall_vga_newline();
                }
            }
            break;
        }
        
        case NODE_IF: {
            int cond = eval_expr(node->data.if_stmt.cond);
            if (cond) {
                for (int i = 0; i < node->data.if_stmt.then_count; i++) {
                    exec_stmt(node->data.if_stmt.then_body[i]);
                }
            } else {
                for (int i = 0; i < node->data.if_stmt.else_count; i++) {
                    exec_stmt(node->data.if_stmt.else_body[i]);
                }
            }
            break;
        }
        
        case NODE_LOOP: {
            int start = eval_expr(node->data.loop.start);
            int end = eval_expr(node->data.loop.end);
            for (int i = start; i <= end; i++) {
                set_var(node->data.loop.var, node->data.loop.var_len, i);
                for (int j = 0; j < node->data.loop.body_count; j++) {
                    exec_stmt(node->data.loop.body[j]);
                }
            }
            break;
        }
        
        default:
            break;
    }
}

/* Initialize JIT */
void jit_init(jit_t *jit, uint8_t *code_buffer) {
    jit->code = code_buffer;
    jit->code_pos = 0;
    jit->had_error = false;
    jit->error_msg = NULL;
    var_count = 0;
}

/* Compile program */
bool jit_compile(jit_t *jit, ast_node_t *program) {
    if (!program || program->type != NODE_PROGRAM) {
        jit->had_error = true;
        jit->error_msg = "Invalid program";
        return false;
    }
    
    /* Execute statements (interpreted for now) */
    for (int i = 0; i < program->data.program.stmt_count; i++) {
        exec_stmt(program->data.program.stmts[i]);
    }
    
    return !jit->had_error;
}

/* Get entry point (not used in interpreted mode) */
jit_func_t jit_get_entry(jit_t *jit) {
    (void)jit;
    return NULL;
}
