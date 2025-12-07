/**
 * Gwango Language - JIT Compiler Header
 */

#ifndef GWANGO_JIT_H
#define GWANGO_JIT_H

#include "parser.h"

/* Max code size */
#define JIT_CODE_SIZE 4096

/* JIT state */
typedef struct {
    uint8_t *code;
    int code_pos;
    bool had_error;
    const char *error_msg;
} jit_t;

/* Function pointer type */
typedef int (*jit_func_t)(void);

/* Initialize JIT */
void jit_init(jit_t *jit, uint8_t *code_buffer);

/* Compile AST to x86 */
bool jit_compile(jit_t *jit, ast_node_t *program);

/* Get entry point */
jit_func_t jit_get_entry(jit_t *jit);

#endif /* GWANGO_JIT_H */
