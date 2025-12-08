/*
 * JSBOX - JavaScript Engine
 * 
 * Parsing: Abstract Syntax Tree Definitions
 */

#ifndef JSBOX_AST_H
#define JSBOX_AST_H

#include "tokens.h"
#include "../diagnostics/source_location.h"
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * AST Node Types
 * ============================================================================ */

typedef enum {
    /* Program */
    JSBOX_AST_PROGRAM,
    
    /* Literals */
    JSBOX_AST_NUMBER_LITERAL,
    JSBOX_AST_STRING_LITERAL,
    JSBOX_AST_BOOL_LITERAL,
    JSBOX_AST_NULL_LITERAL,
    JSBOX_AST_UNDEFINED_LITERAL,
    JSBOX_AST_ARRAY_LITERAL,
    JSBOX_AST_OBJECT_LITERAL,
    
    /* Expressions */
    JSBOX_AST_IDENTIFIER,
    JSBOX_AST_BINARY_EXPR,
    JSBOX_AST_UNARY_EXPR,
    JSBOX_AST_UPDATE_EXPR,        /* ++, -- */
    JSBOX_AST_ASSIGNMENT_EXPR,
    JSBOX_AST_CALL_EXPR,
    JSBOX_AST_MEMBER_EXPR,
    JSBOX_AST_CONDITIONAL_EXPR,   /* ternary ?: */
    JSBOX_AST_SEQUENCE_EXPR,      /* comma operator */
    JSBOX_AST_THIS_EXPR,
    JSBOX_AST_NEW_EXPR,
    JSBOX_AST_FUNCTION_EXPR,
    JSBOX_AST_ARROW_EXPR,
    
    /* Statements */
    JSBOX_AST_BLOCK_STMT,
    JSBOX_AST_EXPR_STMT,
    JSBOX_AST_VAR_DECL,
    JSBOX_AST_FUNCTION_DECL,
    JSBOX_AST_IF_STMT,
    JSBOX_AST_WHILE_STMT,
    JSBOX_AST_DO_WHILE_STMT,
    JSBOX_AST_FOR_STMT,
    JSBOX_AST_FOR_IN_STMT,
    JSBOX_AST_RETURN_STMT,
    JSBOX_AST_BREAK_STMT,
    JSBOX_AST_CONTINUE_STMT,
    JSBOX_AST_THROW_STMT,
    JSBOX_AST_TRY_STMT,
    JSBOX_AST_SWITCH_STMT,
    JSBOX_AST_EMPTY_STMT,
    
    /* Object literal parts */
    JSBOX_AST_PROPERTY,
    
    /* Array patterns */
    JSBOX_AST_SPREAD_ELEMENT
} JSBox_ASTNodeType;

/* ============================================================================
 * Binary Operator Types
 * ============================================================================ */

typedef enum {
    JSBOX_OP_ADD,            /* + */
    JSBOX_OP_SUB,            /* - */
    JSBOX_OP_MUL,            /* * */
    JSBOX_OP_DIV,            /* / */
    JSBOX_OP_MOD,            /* % */
    JSBOX_OP_POW,            /* ** */
    
    JSBOX_OP_EQ,             /* == */
    JSBOX_OP_STRICT_EQ,      /* === */
    JSBOX_OP_NE,             /* != */
    JSBOX_OP_STRICT_NE,      /* !== */
    JSBOX_OP_LT,             /* < */
    JSBOX_OP_GT,             /* > */
    JSBOX_OP_LE,             /* <= */
    JSBOX_OP_GE,             /* >= */
    
    JSBOX_OP_AND,            /* && */
    JSBOX_OP_OR,             /* || */
    JSBOX_OP_NULLISH,        /* ?? */
    
    JSBOX_OP_BIT_AND,        /* & */
    JSBOX_OP_BIT_OR,         /* | */
    JSBOX_OP_BIT_XOR,        /* ^ */
    JSBOX_OP_SHL,            /* << */
    JSBOX_OP_SHR,            /* >> */
    JSBOX_OP_USHR,           /* >>> */
    
    JSBOX_OP_IN,             /* in */
    JSBOX_OP_INSTANCEOF      /* instanceof */
} JSBox_BinaryOp;

/* ============================================================================
 * Unary Operator Types
 * ============================================================================ */

typedef enum {
    JSBOX_UNARY_NEG,         /* - */
    JSBOX_UNARY_POS,         /* + */
    JSBOX_UNARY_NOT,         /* ! */
    JSBOX_UNARY_BIT_NOT,     /* ~ */
    JSBOX_UNARY_TYPEOF,      /* typeof */
    JSBOX_UNARY_VOID,        /* void */
    JSBOX_UNARY_DELETE       /* delete */
} JSBox_UnaryOp;

/* ============================================================================
 * Assignment Operator Types
 * ============================================================================ */

typedef enum {
    JSBOX_ASSIGN,            /* = */
    JSBOX_ASSIGN_ADD,        /* += */
    JSBOX_ASSIGN_SUB,        /* -= */
    JSBOX_ASSIGN_MUL,        /* *= */
    JSBOX_ASSIGN_DIV,        /* /= */
    JSBOX_ASSIGN_MOD         /* %= */
} JSBox_AssignOp;

/* ============================================================================
 * Variable Declaration Kind
 * ============================================================================ */

typedef enum {
    JSBOX_VAR_VAR,
    JSBOX_VAR_LET,
    JSBOX_VAR_CONST
} JSBox_VarKind;

/* ============================================================================
 * AST Node Structure
 * ============================================================================ */

typedef struct JSBox_ASTNode JSBox_ASTNode;

/* Linked list of nodes */
typedef struct JSBox_ASTNodeList {
    JSBox_ASTNode* node;
    struct JSBox_ASTNodeList* next;
} JSBox_ASTNodeList;

/* Variable declarator (name + optional init) */
typedef struct {
    char* name;
    JSBox_ASTNode* init;  /* May be NULL */
} JSBox_VarDeclarator;

/* Object property */
typedef struct {
    JSBox_ASTNode* key;     /* Identifier or string literal */
    JSBox_ASTNode* value;
    bool computed;          /* obj[key] vs obj.key */
    bool shorthand;         /* { x } vs { x: x } */
} JSBox_Property;

struct JSBox_ASTNode {
    JSBox_ASTNodeType type;
    JSBox_SourceSpan span;
    
    union {
        /* JSBOX_AST_PROGRAM, JSBOX_AST_BLOCK_STMT */
        struct {
            JSBox_ASTNodeList* body;
        } program;
        
        /* JSBOX_AST_NUMBER_LITERAL */
        struct {
            double value;
        } number;
        
        /* JSBOX_AST_STRING_LITERAL */
        struct {
            char* value;
        } string;
        
        /* JSBOX_AST_BOOL_LITERAL */
        struct {
            bool value;
        } boolean;
        
        /* JSBOX_AST_IDENTIFIER */
        struct {
            char* name;
        } identifier;
        
        /* JSBOX_AST_BINARY_EXPR */
        struct {
            JSBox_BinaryOp op;
            JSBox_ASTNode* left;
            JSBox_ASTNode* right;
        } binary;
        
        /* JSBOX_AST_UNARY_EXPR */
        struct {
            JSBox_UnaryOp op;
            JSBox_ASTNode* argument;
        } unary;
        
        /* JSBOX_AST_UPDATE_EXPR (++, --) */
        struct {
            bool increment;   /* true for ++, false for -- */
            bool prefix;      /* true for ++x, false for x++ */
            JSBox_ASTNode* argument;
        } update;
        
        /* JSBOX_AST_ASSIGNMENT_EXPR */
        struct {
            JSBox_AssignOp op;
            JSBox_ASTNode* left;
            JSBox_ASTNode* right;
        } assignment;
        
        /* JSBOX_AST_CALL_EXPR */
        struct {
            JSBox_ASTNode* callee;
            JSBox_ASTNodeList* arguments;
        } call;
        
        /* JSBOX_AST_MEMBER_EXPR */
        struct {
            JSBox_ASTNode* object;
            JSBox_ASTNode* property;
            bool computed;    /* obj[prop] vs obj.prop */
        } member;
        
        /* JSBOX_AST_CONDITIONAL_EXPR */
        struct {
            JSBox_ASTNode* test;
            JSBox_ASTNode* consequent;
            JSBox_ASTNode* alternate;
        } conditional;
        
        /* JSBOX_AST_NEW_EXPR */
        struct {
            JSBox_ASTNode* callee;
            JSBox_ASTNodeList* arguments;
        } new_expr;
        
        /* JSBOX_AST_ARRAY_LITERAL */
        struct {
            JSBox_ASTNodeList* elements;
        } array;
        
        /* JSBOX_AST_OBJECT_LITERAL */
        struct {
            JSBox_ASTNodeList* properties;  /* List of JSBOX_AST_PROPERTY nodes */
        } object;
        
        /* JSBOX_AST_PROPERTY */
        JSBox_Property property;
        
        /* JSBOX_AST_VAR_DECL */
        struct {
            JSBox_VarKind kind;
            char* name;
            JSBox_ASTNode* init;
        } var_decl;
        
        /* JSBOX_AST_FUNCTION_DECL, JSBOX_AST_FUNCTION_EXPR */
        struct {
            char* name;              /* NULL for anonymous functions */
            JSBox_ASTNodeList* params;  /* List of identifiers */
            JSBox_ASTNode* body;
        } function;
        
        /* JSBOX_AST_ARROW_EXPR */
        struct {
            JSBox_ASTNodeList* params;
            JSBox_ASTNode* body;     /* Expression or block */
            bool expression;         /* true if body is expression */
        } arrow;
        
        /* JSBOX_AST_IF_STMT */
        struct {
            JSBox_ASTNode* test;
            JSBox_ASTNode* consequent;
            JSBox_ASTNode* alternate;  /* May be NULL */
        } if_stmt;
        
        /* JSBOX_AST_WHILE_STMT, JSBOX_AST_DO_WHILE_STMT */
        struct {
            JSBox_ASTNode* test;
            JSBox_ASTNode* body;
        } while_stmt;
        
        /* JSBOX_AST_FOR_STMT */
        struct {
            JSBox_ASTNode* init;      /* May be NULL */
            JSBox_ASTNode* test;      /* May be NULL */
            JSBox_ASTNode* update;    /* May be NULL */
            JSBox_ASTNode* body;
        } for_stmt;
        
        /* JSBOX_AST_FOR_IN_STMT */
        struct {
            JSBox_ASTNode* left;
            JSBox_ASTNode* right;
            JSBox_ASTNode* body;
        } for_in;
        
        /* JSBOX_AST_RETURN_STMT, JSBOX_AST_THROW_STMT */
        struct {
            JSBox_ASTNode* argument;  /* May be NULL */
        } return_stmt;
        
        /* JSBOX_AST_TRY_STMT */
        struct {
            JSBox_ASTNode* block;
            char* catch_param;
            JSBox_ASTNode* catch_block;
            JSBox_ASTNode* finally_block;
        } try_stmt;
        
        /* JSBOX_AST_SWITCH_STMT */
        struct {
            JSBox_ASTNode* discriminant;
            JSBox_ASTNodeList* cases;
        } switch_stmt;
        
        /* JSBOX_AST_EXPR_STMT */
        struct {
            JSBox_ASTNode* expression;
        } expr_stmt;
        
        /* JSBOX_AST_SEQUENCE_EXPR */
        struct {
            JSBox_ASTNodeList* expressions;
        } sequence;
        
        /* JSBOX_AST_SPREAD_ELEMENT */
        struct {
            JSBox_ASTNode* argument;
        } spread;
    } as;
};

/* ============================================================================
 * AST Creation API
 * ============================================================================ */

/* Allocate a new node */
JSBox_ASTNode* jsbox_ast_alloc(JSBox_ASTNodeType type, JSBox_SourceSpan span);

/* Create a node list */
JSBox_ASTNodeList* jsbox_ast_list_create(JSBox_ASTNode* node);

/* Append to node list */
void jsbox_ast_list_append(JSBox_ASTNodeList** list, JSBox_ASTNode* node);

/* Get length of list */
size_t jsbox_ast_list_length(const JSBox_ASTNodeList* list);

/* Free an AST tree */
void jsbox_ast_free(JSBox_ASTNode* node);

/* Free a node list */
void jsbox_ast_list_free(JSBox_ASTNodeList* list);

/* ============================================================================
 * AST Utilities
 * ============================================================================ */

/* Get node type name for debugging */
const char* jsbox_ast_type_name(JSBox_ASTNodeType type);

/* Get binary operator name */
const char* jsbox_binary_op_name(JSBox_BinaryOp op);

/* Get unary operator name */
const char* jsbox_unary_op_name(JSBox_UnaryOp op);

/* Print AST for debugging */
void jsbox_ast_print(const JSBox_ASTNode* node, int indent);

#endif /* JSBOX_AST_H */
