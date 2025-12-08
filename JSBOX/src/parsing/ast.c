/*
 * JSBOX - JavaScript Engine
 * 
 * Parsing: AST Implementation
 */

#include "ast.h"
#include "../base/memory.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Node Allocation
 * ============================================================================ */

JSBox_ASTNode* jsbox_ast_alloc(JSBox_ASTNodeType type, JSBox_SourceSpan span) {
    JSBox_ASTNode* node = (JSBox_ASTNode*)jsbox_calloc(1, sizeof(JSBox_ASTNode));
    node->type = type;
    node->span = span;
    return node;
}

/* ============================================================================
 * Node List
 * ============================================================================ */

JSBox_ASTNodeList* jsbox_ast_list_create(JSBox_ASTNode* node) {
    JSBox_ASTNodeList* list = (JSBox_ASTNodeList*)jsbox_malloc(sizeof(JSBox_ASTNodeList));
    list->node = node;
    list->next = NULL;
    return list;
}

void jsbox_ast_list_append(JSBox_ASTNodeList** list, JSBox_ASTNode* node) {
    JSBox_ASTNodeList* new_item = jsbox_ast_list_create(node);
    
    if (!*list) {
        *list = new_item;
        return;
    }
    
    JSBox_ASTNodeList* current = *list;
    while (current->next) {
        current = current->next;
    }
    current->next = new_item;
}

size_t jsbox_ast_list_length(const JSBox_ASTNodeList* list) {
    size_t count = 0;
    while (list) {
        count++;
        list = list->next;
    }
    return count;
}

static void free_list_only(JSBox_ASTNodeList* list) {
    while (list) {
        JSBox_ASTNodeList* next = list->next;
        jsbox_free(list);
        list = next;
    }
}

void jsbox_ast_list_free(JSBox_ASTNodeList* list) {
    while (list) {
        JSBox_ASTNodeList* next = list->next;
        jsbox_ast_free(list->node);
        jsbox_free(list);
        list = next;
    }
}

/* ============================================================================
 * Node Freeing
 * ============================================================================ */

void jsbox_ast_free(JSBox_ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case JSBOX_AST_PROGRAM:
        case JSBOX_AST_BLOCK_STMT:
            jsbox_ast_list_free(node->as.program.body);
            break;
            
        case JSBOX_AST_STRING_LITERAL:
            jsbox_free(node->as.string.value);
            break;
            
        case JSBOX_AST_IDENTIFIER:
            jsbox_free(node->as.identifier.name);
            break;
            
        case JSBOX_AST_BINARY_EXPR:
            jsbox_ast_free(node->as.binary.left);
            jsbox_ast_free(node->as.binary.right);
            break;
            
        case JSBOX_AST_UNARY_EXPR:
            jsbox_ast_free(node->as.unary.argument);
            break;
            
        case JSBOX_AST_UPDATE_EXPR:
            jsbox_ast_free(node->as.update.argument);
            break;
            
        case JSBOX_AST_ASSIGNMENT_EXPR:
            jsbox_ast_free(node->as.assignment.left);
            jsbox_ast_free(node->as.assignment.right);
            break;
            
        case JSBOX_AST_CALL_EXPR:
            jsbox_ast_free(node->as.call.callee);
            jsbox_ast_list_free(node->as.call.arguments);
            break;
            
        case JSBOX_AST_MEMBER_EXPR:
            jsbox_ast_free(node->as.member.object);
            jsbox_ast_free(node->as.member.property);
            break;
            
        case JSBOX_AST_CONDITIONAL_EXPR:
            jsbox_ast_free(node->as.conditional.test);
            jsbox_ast_free(node->as.conditional.consequent);
            jsbox_ast_free(node->as.conditional.alternate);
            break;
            
        case JSBOX_AST_NEW_EXPR:
            jsbox_ast_free(node->as.new_expr.callee);
            jsbox_ast_list_free(node->as.new_expr.arguments);
            break;
            
        case JSBOX_AST_ARRAY_LITERAL:
            jsbox_ast_list_free(node->as.array.elements);
            break;
            
        case JSBOX_AST_OBJECT_LITERAL:
            jsbox_ast_list_free(node->as.object.properties);
            break;
            
        case JSBOX_AST_PROPERTY:
            jsbox_ast_free(node->as.property.key);
            jsbox_ast_free(node->as.property.value);
            break;
            
        case JSBOX_AST_VAR_DECL:
            jsbox_free(node->as.var_decl.name);
            jsbox_ast_free(node->as.var_decl.init);
            break;
            
        case JSBOX_AST_FUNCTION_DECL:
        case JSBOX_AST_FUNCTION_EXPR:
            jsbox_free(node->as.function.name);
            /* Params are identifiers with allocated names */
            jsbox_ast_list_free(node->as.function.params);
            jsbox_ast_free(node->as.function.body);
            break;
            
        case JSBOX_AST_ARROW_EXPR:
            jsbox_ast_list_free(node->as.arrow.params);
            jsbox_ast_free(node->as.arrow.body);
            break;
            
        case JSBOX_AST_IF_STMT:
            jsbox_ast_free(node->as.if_stmt.test);
            jsbox_ast_free(node->as.if_stmt.consequent);
            jsbox_ast_free(node->as.if_stmt.alternate);
            break;
            
        case JSBOX_AST_WHILE_STMT:
        case JSBOX_AST_DO_WHILE_STMT:
            jsbox_ast_free(node->as.while_stmt.test);
            jsbox_ast_free(node->as.while_stmt.body);
            break;
            
        case JSBOX_AST_FOR_STMT:
            jsbox_ast_free(node->as.for_stmt.init);
            jsbox_ast_free(node->as.for_stmt.test);
            jsbox_ast_free(node->as.for_stmt.update);
            jsbox_ast_free(node->as.for_stmt.body);
            break;
            
        case JSBOX_AST_FOR_IN_STMT:
            jsbox_ast_free(node->as.for_in.left);
            jsbox_ast_free(node->as.for_in.right);
            jsbox_ast_free(node->as.for_in.body);
            break;
            
        case JSBOX_AST_RETURN_STMT:
        case JSBOX_AST_THROW_STMT:
            jsbox_ast_free(node->as.return_stmt.argument);
            break;
            
        case JSBOX_AST_TRY_STMT:
            jsbox_ast_free(node->as.try_stmt.block);
            jsbox_free(node->as.try_stmt.catch_param);
            jsbox_ast_free(node->as.try_stmt.catch_block);
            jsbox_ast_free(node->as.try_stmt.finally_block);
            break;
            
        case JSBOX_AST_SWITCH_STMT:
            jsbox_ast_free(node->as.switch_stmt.discriminant);
            jsbox_ast_list_free(node->as.switch_stmt.cases);
            break;
            
        case JSBOX_AST_EXPR_STMT:
            jsbox_ast_free(node->as.expr_stmt.expression);
            break;
            
        case JSBOX_AST_SEQUENCE_EXPR:
            jsbox_ast_list_free(node->as.sequence.expressions);
            break;
            
        case JSBOX_AST_SPREAD_ELEMENT:
            jsbox_ast_free(node->as.spread.argument);
            break;
            
        default:
            /* No dynamic memory to free */
            break;
    }
    
    jsbox_free(node);
}

/* ============================================================================
 * Debug Printing
 * ============================================================================ */

const char* jsbox_ast_type_name(JSBox_ASTNodeType type) {
    switch (type) {
        case JSBOX_AST_PROGRAM:           return "Program";
        case JSBOX_AST_NUMBER_LITERAL:    return "NumberLiteral";
        case JSBOX_AST_STRING_LITERAL:    return "StringLiteral";
        case JSBOX_AST_BOOL_LITERAL:      return "BoolLiteral";
        case JSBOX_AST_NULL_LITERAL:      return "NullLiteral";
        case JSBOX_AST_UNDEFINED_LITERAL: return "UndefinedLiteral";
        case JSBOX_AST_ARRAY_LITERAL:     return "ArrayLiteral";
        case JSBOX_AST_OBJECT_LITERAL:    return "ObjectLiteral";
        case JSBOX_AST_IDENTIFIER:        return "Identifier";
        case JSBOX_AST_BINARY_EXPR:       return "BinaryExpr";
        case JSBOX_AST_UNARY_EXPR:        return "UnaryExpr";
        case JSBOX_AST_UPDATE_EXPR:       return "UpdateExpr";
        case JSBOX_AST_ASSIGNMENT_EXPR:   return "AssignmentExpr";
        case JSBOX_AST_CALL_EXPR:         return "CallExpr";
        case JSBOX_AST_MEMBER_EXPR:       return "MemberExpr";
        case JSBOX_AST_CONDITIONAL_EXPR:  return "ConditionalExpr";
        case JSBOX_AST_SEQUENCE_EXPR:     return "SequenceExpr";
        case JSBOX_AST_THIS_EXPR:         return "ThisExpr";
        case JSBOX_AST_NEW_EXPR:          return "NewExpr";
        case JSBOX_AST_FUNCTION_EXPR:     return "FunctionExpr";
        case JSBOX_AST_ARROW_EXPR:        return "ArrowExpr";
        case JSBOX_AST_BLOCK_STMT:        return "BlockStmt";
        case JSBOX_AST_EXPR_STMT:         return "ExprStmt";
        case JSBOX_AST_VAR_DECL:          return "VarDecl";
        case JSBOX_AST_FUNCTION_DECL:     return "FunctionDecl";
        case JSBOX_AST_IF_STMT:           return "IfStmt";
        case JSBOX_AST_WHILE_STMT:        return "WhileStmt";
        case JSBOX_AST_DO_WHILE_STMT:     return "DoWhileStmt";
        case JSBOX_AST_FOR_STMT:          return "ForStmt";
        case JSBOX_AST_FOR_IN_STMT:       return "ForInStmt";
        case JSBOX_AST_RETURN_STMT:       return "ReturnStmt";
        case JSBOX_AST_BREAK_STMT:        return "BreakStmt";
        case JSBOX_AST_CONTINUE_STMT:     return "ContinueStmt";
        case JSBOX_AST_THROW_STMT:        return "ThrowStmt";
        case JSBOX_AST_TRY_STMT:          return "TryStmt";
        case JSBOX_AST_SWITCH_STMT:       return "SwitchStmt";
        case JSBOX_AST_EMPTY_STMT:        return "EmptyStmt";
        case JSBOX_AST_PROPERTY:          return "Property";
        case JSBOX_AST_SPREAD_ELEMENT:    return "SpreadElement";
        default:                          return "Unknown";
    }
}

const char* jsbox_binary_op_name(JSBox_BinaryOp op) {
    switch (op) {
        case JSBOX_OP_ADD:        return "+";
        case JSBOX_OP_SUB:        return "-";
        case JSBOX_OP_MUL:        return "*";
        case JSBOX_OP_DIV:        return "/";
        case JSBOX_OP_MOD:        return "%";
        case JSBOX_OP_POW:        return "**";
        case JSBOX_OP_EQ:         return "==";
        case JSBOX_OP_STRICT_EQ:  return "===";
        case JSBOX_OP_NE:         return "!=";
        case JSBOX_OP_STRICT_NE:  return "!==";
        case JSBOX_OP_LT:         return "<";
        case JSBOX_OP_GT:         return ">";
        case JSBOX_OP_LE:         return "<=";
        case JSBOX_OP_GE:         return ">=";
        case JSBOX_OP_AND:        return "&&";
        case JSBOX_OP_OR:         return "||";
        case JSBOX_OP_NULLISH:    return "??";
        case JSBOX_OP_BIT_AND:    return "&";
        case JSBOX_OP_BIT_OR:     return "|";
        case JSBOX_OP_BIT_XOR:    return "^";
        case JSBOX_OP_SHL:        return "<<";
        case JSBOX_OP_SHR:        return ">>";
        case JSBOX_OP_USHR:       return ">>>";
        case JSBOX_OP_IN:         return "in";
        case JSBOX_OP_INSTANCEOF: return "instanceof";
        default:                  return "?";
    }
}

const char* jsbox_unary_op_name(JSBox_UnaryOp op) {
    switch (op) {
        case JSBOX_UNARY_NEG:     return "-";
        case JSBOX_UNARY_POS:     return "+";
        case JSBOX_UNARY_NOT:     return "!";
        case JSBOX_UNARY_BIT_NOT: return "~";
        case JSBOX_UNARY_TYPEOF:  return "typeof";
        case JSBOX_UNARY_VOID:    return "void";
        case JSBOX_UNARY_DELETE:  return "delete";
        default:                  return "?";
    }
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void jsbox_ast_print(const JSBox_ASTNode* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    printf("%s", jsbox_ast_type_name(node->type));
    
    switch (node->type) {
        case JSBOX_AST_NUMBER_LITERAL:
            printf(" %g\n", node->as.number.value);
            break;
            
        case JSBOX_AST_STRING_LITERAL:
            printf(" \"%s\"\n", node->as.string.value);
            break;
            
        case JSBOX_AST_BOOL_LITERAL:
            printf(" %s\n", node->as.boolean.value ? "true" : "false");
            break;
            
        case JSBOX_AST_IDENTIFIER:
            printf(" '%s'\n", node->as.identifier.name);
            break;
            
        case JSBOX_AST_BINARY_EXPR:
            printf(" %s\n", jsbox_binary_op_name(node->as.binary.op));
            jsbox_ast_print(node->as.binary.left, indent + 1);
            jsbox_ast_print(node->as.binary.right, indent + 1);
            break;
            
        case JSBOX_AST_UNARY_EXPR:
            printf(" %s\n", jsbox_unary_op_name(node->as.unary.op));
            jsbox_ast_print(node->as.unary.argument, indent + 1);
            break;
            
        case JSBOX_AST_CALL_EXPR:
            printf("\n");
            print_indent(indent + 1);
            printf("callee:\n");
            jsbox_ast_print(node->as.call.callee, indent + 2);
            print_indent(indent + 1);
            printf("args:\n");
            for (JSBox_ASTNodeList* arg = node->as.call.arguments; arg; arg = arg->next) {
                jsbox_ast_print(arg->node, indent + 2);
            }
            break;
            
        case JSBOX_AST_VAR_DECL:
            printf(" %s\n", node->as.var_decl.name);
            if (node->as.var_decl.init) {
                jsbox_ast_print(node->as.var_decl.init, indent + 1);
            }
            break;
            
        case JSBOX_AST_PROGRAM:
        case JSBOX_AST_BLOCK_STMT:
            printf("\n");
            for (JSBox_ASTNodeList* stmt = node->as.program.body; stmt; stmt = stmt->next) {
                jsbox_ast_print(stmt->node, indent + 1);
            }
            break;
            
        default:
            printf("\n");
            break;
    }
}
