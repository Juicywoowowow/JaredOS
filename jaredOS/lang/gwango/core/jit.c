/**
 * Gwango Language - JIT Compiler Implementation
 * 
 * Compiles AST directly to x86 machine code
 */

#include "jit.h"
#include "../../../kernel/lib/string.h"
#include "../../../kernel/drivers/vga.h"
#include "../../../kernel/drivers/keyboard.h"
#include "../../../kernel/lib/printf.h"

/* Variables storage - still used for runtime */
#define MAX_VARS 64
static struct {
    const char *name;
    int name_len;
    int stack_offset;  /* Offset from EBP */
} variables[MAX_VARS];
static int var_count = 0;
static int stack_size = 0;

/* Current JIT state */
static jit_t *current_jit = NULL;

/* Emit helpers */
static void emit_byte(uint8_t b) {
    if (current_jit->code_pos < JIT_CODE_SIZE) {
        current_jit->code[current_jit->code_pos++] = b;
    }
}

static void emit_word(uint16_t w) {
    emit_byte(w & 0xFF);
    emit_byte((w >> 8) & 0xFF);
}

static void emit_dword(uint32_t d) {
    emit_byte(d & 0xFF);
    emit_byte((d >> 8) & 0xFF);
    emit_byte((d >> 16) & 0xFF);
    emit_byte((d >> 24) & 0xFF);
}

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

/* Allocate variable on stack */
static int alloc_var(const char *name, int len) {
    int idx = var_count++;
    variables[idx].name = name;
    variables[idx].name_len = len;
    stack_size += 4;
    variables[idx].stack_offset = -stack_size;
    return idx;
}

/* Kernel call handlers (C functions that JIT code calls) */
/* Must use 'used' and 'noinline' to prevent optimization since we only take addresses */
__attribute__((used, noinline))
void kcall_vga_print_num(int n) {
    kprintf("%d", n);
}

__attribute__((used, noinline))
void kcall_vga_print_str(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        vga_putchar(s[i]);
    }
}

__attribute__((used, noinline))
void kcall_vga_clear(void) {
    vga_clear();
}

__attribute__((used, noinline))
void kcall_vga_newline(void) {
    kprintf("\n");
}

/* @kb module */
__attribute__((used, noinline))
int kcall_kb_getchar(void) {
    return (int)keyboard_getchar();
}

__attribute__((used, noinline))
int kcall_kb_haskey(void) {
    return keyboard_has_key() ? 1 : 0;
}

/* @sys module */
__attribute__((used, noinline))
int kcall_sys_time(void) {
    extern uint32_t timer_get_ticks(void);
    return (int)timer_get_ticks();
}

__attribute__((used, noinline))
void kcall_sys_sleep(int ticks) {
    extern uint32_t timer_get_ticks(void);
    uint32_t start = timer_get_ticks();
    while (timer_get_ticks() - start < (uint32_t)ticks);
}

__attribute__((used, noinline))
void kcall_sys_reboot(void) {
    /* Keyboard controller reset */
    uint8_t good = 0x02;
    while (good & 0x02)
        __asm__ volatile("inb $0x64, %0" : "=a"(good));
    __asm__ volatile("outb %0, $0x64" : : "a"((uint8_t)0xFE));
}

/* @mem module */
__attribute__((used, noinline))
int kcall_mem_peek(int addr) {
    return *(uint8_t*)(uint32_t)addr;
}

__attribute__((used, noinline))
void kcall_mem_poke(int addr, int val) {
    *(uint8_t*)(uint32_t)addr = (uint8_t)val;
}

/* Forward declarations */
static void compile_expr(ast_node_t *node);
static void compile_stmt(ast_node_t *node);

/* Compile expression - result in EAX */
static void compile_expr(ast_node_t *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_NUMBER:
            /* mov eax, imm32 */
            emit_byte(0xB8);
            emit_dword(node->data.number);
            break;
            
        case NODE_IDENT: {
            int idx = find_var(node->data.string.str, node->data.string.len);
            if (idx >= 0) {
                /* mov eax, [ebp + offset] */
                emit_byte(0x8B);
                emit_byte(0x45);
                emit_byte((uint8_t)variables[idx].stack_offset);
            } else {
                /* Unknown var, load 0 */
                emit_byte(0xB8);
                emit_dword(0);
            }
            break;
        }
            
        case NODE_BINARY: {
            /* Compile left, push, compile right, pop ebx, op */
            compile_expr(node->data.binary.left);
            /* push eax */
            emit_byte(0x50);
            compile_expr(node->data.binary.right);
            /* pop ebx */
            emit_byte(0x5B);
            
            switch (node->data.binary.op) {
                case TOK_PLUS:
                    /* add eax, ebx */
                    emit_byte(0x01);
                    emit_byte(0xD8);
                    break;
                case TOK_MINUS:
                    /* sub ebx, eax; mov eax, ebx */
                    emit_byte(0x29);
                    emit_byte(0xC3);
                    emit_byte(0x89);
                    emit_byte(0xD8);
                    break;
                case TOK_STAR:
                    /* imul eax, ebx */
                    emit_byte(0x0F);
                    emit_byte(0xAF);
                    emit_byte(0xC3);
                    break;
                case TOK_SLASH:
                    /* xchg eax, ebx; cdq; idiv ebx */
                    emit_byte(0x93);        /* xchg eax, ebx */
                    emit_byte(0x99);        /* cdq */
                    emit_byte(0xF7);        /* idiv ebx */
                    emit_byte(0xFB);
                    break;
                case TOK_LT:
                    /* cmp ebx, eax; setl al; movzx eax, al */
                    emit_byte(0x39); emit_byte(0xC3);  /* cmp ebx, eax */
                    emit_byte(0x0F); emit_byte(0x9C); emit_byte(0xC0);  /* setl al */
                    emit_byte(0x0F); emit_byte(0xB6); emit_byte(0xC0);  /* movzx eax, al */
                    break;
                case TOK_GT:
                    /* cmp ebx, eax; setg al; movzx eax, al */
                    emit_byte(0x39); emit_byte(0xC3);
                    emit_byte(0x0F); emit_byte(0x9F); emit_byte(0xC0);
                    emit_byte(0x0F); emit_byte(0xB6); emit_byte(0xC0);
                    break;
                case TOK_EQEQ:
                    /* cmp ebx, eax; sete al; movzx eax, al */
                    emit_byte(0x39); emit_byte(0xC3);
                    emit_byte(0x0F); emit_byte(0x94); emit_byte(0xC0);
                    emit_byte(0x0F); emit_byte(0xB6); emit_byte(0xC0);
                    break;
                default:
                    break;
            }
            break;
        }
        
        case NODE_UNARY:
            compile_expr(node->data.binary.right);
            /* neg eax */
            emit_byte(0xF7);
            emit_byte(0xD8);
            break;
        
        /* Handle kernel calls that return values */
        case NODE_KCALL: {
            const char *mod = node->data.call.module;
            int mod_len = node->data.call.module_len;
            const char *fn = node->data.call.name;
            int fn_len = node->data.call.name_len;
            
            /* @kb module - functions return int */
            if (mod_len == 2 && mod[0] == 'k' && mod[1] == 'b') {
                if (fn_len == 7 && strncmp(fn, "getchar", 7) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_kb_getchar);
                    emit_byte(0xFF);
                    emit_byte(0xD0);  /* Result in EAX */
                } else if (fn_len == 6 && strncmp(fn, "haskey", 6) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_kb_haskey);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                }
            }
            /* @sys module */
            else if (mod_len == 3 && mod[0] == 's' && mod[1] == 'y' && mod[2] == 's') {
                if (fn_len == 4 && strncmp(fn, "time", 4) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_sys_time);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                }
            }
            /* @mem module */
            else if (mod_len == 3 && mod[0] == 'm' && mod[1] == 'e' && mod[2] == 'm') {
                if (fn_len == 4 && strncmp(fn, "peek", 4) == 0) {
                    if (node->data.call.arg_count > 0) {
                        compile_expr(node->data.call.args[0]);
                        emit_byte(0x50);
                        emit_byte(0xB8);
                        emit_dword((uint32_t)kcall_mem_peek);
                        emit_byte(0xFF);
                        emit_byte(0xD0);
                        emit_byte(0x83);
                        emit_byte(0xC4);
                        emit_byte(0x04);
                    }
                }
            }
            break;
        }
            
        default:
            /* Unknown node - emit mov eax, 0 as fallback */
            emit_byte(0xB8);
            emit_dword(0);
            break;
    }
}

/* Compile statement */
static void compile_stmt(ast_node_t *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_VAR_DECL: {
            /* Compile value into EAX */
            compile_expr(node->data.var_decl.value);
            /* Allocate var */
            int idx = alloc_var(node->data.var_decl.name, node->data.var_decl.name_len);
            /* mov [ebp + offset], eax */
            emit_byte(0x89);
            emit_byte(0x45);
            emit_byte((uint8_t)variables[idx].stack_offset);
            break;
        }
        
        case NODE_ASSIGN: {
            compile_expr(node->data.var_decl.value);
            int idx = find_var(node->data.var_decl.name, node->data.var_decl.name_len);
            if (idx >= 0) {
                /* mov [ebp + offset], eax */
                emit_byte(0x89);
                emit_byte(0x45);
                emit_byte((uint8_t)variables[idx].stack_offset);
            }
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
                            /* push len */
                            emit_byte(0x68);
                            emit_dword(arg->data.string.len);
                            /* push str ptr */
                            emit_byte(0x68);
                            emit_dword((uint32_t)arg->data.string.str);
                            /* mov eax, func; call eax */
                            emit_byte(0xB8);
                            emit_dword((uint32_t)kcall_vga_print_str);
                            emit_byte(0xFF);
                            emit_byte(0xD0);
                            /* add esp, 8 */
                            emit_byte(0x83);
                            emit_byte(0xC4);
                            emit_byte(0x08);
                        } else {
                            /* Compile expression */
                            compile_expr(arg);
                            /* push eax */
                            emit_byte(0x50);
                            /* mov eax, func; call eax */
                            emit_byte(0xB8);
                            emit_dword((uint32_t)kcall_vga_print_num);
                            emit_byte(0xFF);
                            emit_byte(0xD0);
                            /* add esp, 4 */
                            emit_byte(0x83);
                            emit_byte(0xC4);
                            emit_byte(0x04);
                        }
                    }
                } else if (fn_len == 5 && strncmp(fn, "clear", 5) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_vga_clear);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                } else if (fn_len == 7 && strncmp(fn, "newline", 7) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_vga_newline);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                }
            }
            /* @kb module */
            else if (mod_len == 2 && mod[0] == 'k' && mod[1] == 'b') {
                if (fn_len == 7 && strncmp(fn, "getchar", 7) == 0) {
                    /* call kcall_kb_getchar, result in eax */
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_kb_getchar);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                } else if (fn_len == 6 && strncmp(fn, "haskey", 6) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_kb_haskey);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                }
            }
            /* @sys module */
            else if (mod_len == 3 && mod[0] == 's' && mod[1] == 'y' && mod[2] == 's') {
                if (fn_len == 4 && strncmp(fn, "time", 4) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_sys_time);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                } else if (fn_len == 5 && strncmp(fn, "sleep", 5) == 0) {
                    if (node->data.call.arg_count > 0) {
                        compile_expr(node->data.call.args[0]);
                        emit_byte(0x50);  /* push eax */
                        emit_byte(0xB8);
                        emit_dword((uint32_t)kcall_sys_sleep);
                        emit_byte(0xFF);
                        emit_byte(0xD0);
                        emit_byte(0x83);
                        emit_byte(0xC4);
                        emit_byte(0x04);
                    }
                } else if (fn_len == 6 && strncmp(fn, "reboot", 6) == 0) {
                    emit_byte(0xB8);
                    emit_dword((uint32_t)kcall_sys_reboot);
                    emit_byte(0xFF);
                    emit_byte(0xD0);
                }
            }
            /* @mem module */
            else if (mod_len == 3 && mod[0] == 'm' && mod[1] == 'e' && mod[2] == 'm') {
                if (fn_len == 4 && strncmp(fn, "peek", 4) == 0) {
                    if (node->data.call.arg_count > 0) {
                        compile_expr(node->data.call.args[0]);
                        emit_byte(0x50);
                        emit_byte(0xB8);
                        emit_dword((uint32_t)kcall_mem_peek);
                        emit_byte(0xFF);
                        emit_byte(0xD0);
                        emit_byte(0x83);
                        emit_byte(0xC4);
                        emit_byte(0x04);
                    }
                } else if (fn_len == 4 && strncmp(fn, "poke", 4) == 0) {
                    if (node->data.call.arg_count >= 2) {
                        compile_expr(node->data.call.args[1]);  /* val */
                        emit_byte(0x50);
                        compile_expr(node->data.call.args[0]);  /* addr */
                        emit_byte(0x50);
                        emit_byte(0xB8);
                        emit_dword((uint32_t)kcall_mem_poke);
                        emit_byte(0xFF);
                        emit_byte(0xD0);
                        emit_byte(0x83);
                        emit_byte(0xC4);
                        emit_byte(0x08);
                    }
                }
            }
            break;
        }
        
        case NODE_IF: {
            /* Compile condition */
            compile_expr(node->data.if_stmt.cond);
            /* test eax, eax */
            emit_byte(0x85);
            emit_byte(0xC0);
            /* jz else_label (will patch) */
            emit_byte(0x0F);
            emit_byte(0x84);
            int jz_pos = current_jit->code_pos;
            emit_dword(0);  /* placeholder */
            
            /* Then block */
            for (int i = 0; i < node->data.if_stmt.then_count; i++) {
                compile_stmt(node->data.if_stmt.then_body[i]);
            }
            
            if (node->data.if_stmt.else_count > 0) {
                /* jmp end_label */
                emit_byte(0xE9);
                int jmp_pos = current_jit->code_pos;
                emit_dword(0);
                
                /* Patch jz to here */
                int else_label = current_jit->code_pos;
                *(uint32_t*)&current_jit->code[jz_pos] = else_label - (jz_pos + 4);
                
                /* Else block */
                for (int i = 0; i < node->data.if_stmt.else_count; i++) {
                    compile_stmt(node->data.if_stmt.else_body[i]);
                }
                
                /* Patch jmp to here */
                int end_label = current_jit->code_pos;
                *(uint32_t*)&current_jit->code[jmp_pos] = end_label - (jmp_pos + 4);
            } else {
                /* Patch jz to here */
                int end_label = current_jit->code_pos;
                *(uint32_t*)&current_jit->code[jz_pos] = end_label - (jz_pos + 4);
            }
            break;
        }
        
        case NODE_LOOP: {
            /* Allocate loop var */
            int idx = alloc_var(node->data.loop.var, node->data.loop.var_len);
            
            /* Init: compile start, store to var */
            compile_expr(node->data.loop.start);
            emit_byte(0x89);
            emit_byte(0x45);
            emit_byte((uint8_t)variables[idx].stack_offset);
            
            /* Loop start label */
            int loop_start = current_jit->code_pos;
            
            /* Check: var <= end */
            emit_byte(0x8B);
            emit_byte(0x45);
            emit_byte((uint8_t)variables[idx].stack_offset);  /* mov eax, [ebp+off] */
            emit_byte(0x50);  /* push eax */
            compile_expr(node->data.loop.end);
            emit_byte(0x5B);  /* pop ebx */
            /* cmp ebx, eax; jg loop_end */
            emit_byte(0x39);
            emit_byte(0xC3);
            emit_byte(0x0F);
            emit_byte(0x8F);
            int jg_pos = current_jit->code_pos;
            emit_dword(0);
            
            /* Body */
            for (int i = 0; i < node->data.loop.body_count; i++) {
                compile_stmt(node->data.loop.body[i]);
            }
            
            /* Increment var */
            emit_byte(0x8B);
            emit_byte(0x45);
            emit_byte((uint8_t)variables[idx].stack_offset);
            emit_byte(0x40);  /* inc eax */
            emit_byte(0x89);
            emit_byte(0x45);
            emit_byte((uint8_t)variables[idx].stack_offset);
            
            /* jmp loop_start */
            emit_byte(0xE9);
            emit_dword(loop_start - (current_jit->code_pos + 4));
            
            /* Patch jg */
            *(uint32_t*)&current_jit->code[jg_pos] = current_jit->code_pos - (jg_pos + 4);
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
    current_jit = jit;
    var_count = 0;
    stack_size = 0;
}

/* Compile program */
bool jit_compile(jit_t *jit, ast_node_t *program) {
    if (!program || program->type != NODE_PROGRAM) {
        jit->had_error = true;
        jit->error_msg = "Invalid program";
        return false;
    }
    
    current_jit = jit;
    
    /* Prologue */
    emit_byte(0x55);              /* push ebp */
    emit_byte(0x89);              /* mov ebp, esp */
    emit_byte(0xE5);
    emit_byte(0x83);              /* sub esp, 64 (reserve stack) */
    emit_byte(0xEC);
    emit_byte(0x40);
    
    /* Compile statements */
    for (int i = 0; i < program->data.program.stmt_count; i++) {
        compile_stmt(program->data.program.stmts[i]);
    }
    
    /* Epilogue */
    emit_byte(0x89);              /* mov esp, ebp */
    emit_byte(0xEC);
    emit_byte(0x5D);              /* pop ebp */
    emit_byte(0xC3);              /* ret */
    
    return !jit->had_error;
}

/* Disassemble generated code */
void jit_disassemble(jit_t *jit) {
    uint8_t *code = jit->code;
    int i = 0;
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("Disassembly (%d bytes):\n", jit->code_pos);
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    while (i < jit->code_pos) {
        vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
        kprintf("%04X: ", i);
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        
        uint8_t op = code[i];
        
        if (op == 0x55) {
            kprintf("push ebp\n");
            i++;
        } else if (op == 0x89 && code[i+1] == 0xE5) {
            kprintf("mov ebp, esp\n");
            i += 2;
        } else if (op == 0x83 && code[i+1] == 0xEC) {
            kprintf("sub esp, %d\n", code[i+2]);
            i += 3;
        } else if (op == 0xB8) {
            uint32_t val = *(uint32_t*)&code[i+1];
            kprintf("mov eax, %d (0x%x)\n", val, val);
            i += 5;
        } else if (op == 0x8B && code[i+1] == 0x45) {
            int8_t off = (int8_t)code[i+2];
            kprintf("mov eax, [ebp%c%d]\n", off < 0 ? '-' : '+', off < 0 ? -off : off);
            i += 3;
        } else if (op == 0x89 && code[i+1] == 0x45) {
            int8_t off = (int8_t)code[i+2];
            kprintf("mov [ebp%c%d], eax\n", off < 0 ? '-' : '+', off < 0 ? -off : off);
            i += 3;
        } else if (op == 0x50) {
            kprintf("push eax\n");
            i++;
        } else if (op == 0x5B) {
            kprintf("pop ebx\n");
            i++;
        } else if (op == 0x01 && code[i+1] == 0xD8) {
            kprintf("add eax, ebx\n");
            i += 2;
        } else if (op == 0x29 && code[i+1] == 0xC3) {
            kprintf("sub ebx, eax\n");
            kprintf("      mov eax, ebx\n"); // The move follows immediately in our codegen
            i += 4;
        } else if (op == 0x0F && code[i+1] == 0xAF) {
            kprintf("imul eax, ebx\n");
            i += 3;
        } else if (op == 0x93) {
            kprintf("xchg eax, ebx\n");
            i++;
        } else if (op == 0x99) {
            kprintf("cdq\n");
            i++;
        } else if (op == 0xF7 && code[i+1] == 0xFB) {
            kprintf("idiv ebx\n");
            i += 2;
        } else if (op == 0xF7 && code[i+1] == 0xD8) {
            kprintf("neg eax\n");
            i += 2;
        } else if (op == 0x39 && code[i+1] == 0xC3) {
            kprintf("cmp ebx, eax\n");
            i += 2;
        } else if (op == 0x0F && (code[i+1] & 0xF0) == 0x90) {
            kprintf("set%s al\n", 
                code[i+1] == 0x9C ? "l" : 
                code[i+1] == 0x9F ? "g" : 
                code[i+1] == 0x94 ? "e" : "cc");
            i += 3; // setcc + movzx
        } else if (op == 0x68) {
            uint32_t val = *(uint32_t*)&code[i+1];
            kprintf("push 0x%x\n", val);
            i += 5;
        } else if (op == 0xFF && code[i+1] == 0xD0) {
            kprintf("call eax\n");
            i += 2;
        } else if (op == 0x83 && code[i+1] == 0xC4) {
            kprintf("add esp, %d\n", code[i+2]);
            i += 3;
        } else if (op == 0x85 && code[i+1] == 0xC0) {
            kprintf("test eax, eax\n");
            i += 2;
        } else if (op == 0x0F && code[i+1] == 0x84) {
            int32_t rel = *(int32_t*)&code[i+2];
            kprintf("jz 0x%x\n", i + 6 + rel);
            i += 6;
        } else if (op == 0xE9) {
            int32_t rel = *(int32_t*)&code[i+1];
            kprintf("jmp 0x%x\n", i + 5 + rel);
            i += 5;
        } else if (op == 0x0F && code[i+1] == 0x8F) {
            int32_t rel = *(int32_t*)&code[i+2];
            kprintf("jg 0x%x\n", i + 6 + rel);
            i += 6;
        } else if (op == 0x40) {
            kprintf("inc eax\n");
            i++;
        } else if (op == 0x89 && code[i+1] == 0xEC) {
            kprintf("mov esp, ebp\n");
            i += 2;
        } else if (op == 0x5D) {
            kprintf("pop ebp\n");
            i++;
        } else if (op == 0xC3) {
            kprintf("ret\n");
            i++;
        } else {
            kprintf("db 0x%02X\n", op);
            i++;
        }
    }
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* Get entry point */
jit_func_t jit_get_entry(jit_t *jit) {
    return (jit_func_t)jit->code;
}
