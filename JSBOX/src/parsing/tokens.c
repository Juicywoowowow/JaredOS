/*
 * JSBOX - JavaScript Engine
 * 
 * Parsing: Token Helpers Implementation
 */

#include "tokens.h"
#include "../base/memory.h"
#include <string.h>
#include <stdio.h>

const char* jsbox_token_type_name(JSBox_TokenType type) {
    switch (type) {
        case JSBOX_TOK_EOF:           return "EOF";
        case JSBOX_TOK_ERROR:         return "ERROR";
        case JSBOX_TOK_NUMBER:        return "NUMBER";
        case JSBOX_TOK_STRING:        return "STRING";
        case JSBOX_TOK_IDENTIFIER:    return "IDENTIFIER";
        case JSBOX_TOK_VAR:           return "var";
        case JSBOX_TOK_LET:           return "let";
        case JSBOX_TOK_CONST:         return "const";
        case JSBOX_TOK_FUNCTION:      return "function";
        case JSBOX_TOK_RETURN:        return "return";
        case JSBOX_TOK_IF:            return "if";
        case JSBOX_TOK_ELSE:          return "else";
        case JSBOX_TOK_WHILE:         return "while";
        case JSBOX_TOK_FOR:           return "for";
        case JSBOX_TOK_DO:            return "do";
        case JSBOX_TOK_BREAK:         return "break";
        case JSBOX_TOK_CONTINUE:      return "continue";
        case JSBOX_TOK_NEW:           return "new";
        case JSBOX_TOK_THIS:          return "this";
        case JSBOX_TOK_TRUE:          return "true";
        case JSBOX_TOK_FALSE:         return "false";
        case JSBOX_TOK_NULL:          return "null";
        case JSBOX_TOK_UNDEFINED:     return "undefined";
        case JSBOX_TOK_TYPEOF:        return "typeof";
        case JSBOX_TOK_INSTANCEOF:    return "instanceof";
        case JSBOX_TOK_IN:            return "in";
        case JSBOX_TOK_DELETE:        return "delete";
        case JSBOX_TOK_VOID:          return "void";
        case JSBOX_TOK_TRY:           return "try";
        case JSBOX_TOK_CATCH:         return "catch";
        case JSBOX_TOK_FINALLY:       return "finally";
        case JSBOX_TOK_THROW:         return "throw";
        case JSBOX_TOK_SWITCH:        return "switch";
        case JSBOX_TOK_CASE:          return "case";
        case JSBOX_TOK_DEFAULT:       return "default";
        case JSBOX_TOK_CLASS:         return "class";
        case JSBOX_TOK_EXTENDS:       return "extends";
        case JSBOX_TOK_SUPER:         return "super";
        case JSBOX_TOK_IMPORT:        return "import";
        case JSBOX_TOK_EXPORT:        return "export";
        case JSBOX_TOK_PLUS:          return "+";
        case JSBOX_TOK_MINUS:         return "-";
        case JSBOX_TOK_STAR:          return "*";
        case JSBOX_TOK_SLASH:         return "/";
        case JSBOX_TOK_PERCENT:       return "%";
        case JSBOX_TOK_STAR_STAR:     return "**";
        case JSBOX_TOK_PLUS_PLUS:     return "++";
        case JSBOX_TOK_MINUS_MINUS:   return "--";
        case JSBOX_TOK_EQ:            return "=";
        case JSBOX_TOK_PLUS_EQ:       return "+=";
        case JSBOX_TOK_MINUS_EQ:      return "-=";
        case JSBOX_TOK_STAR_EQ:       return "*=";
        case JSBOX_TOK_SLASH_EQ:      return "/=";
        case JSBOX_TOK_PERCENT_EQ:    return "%=";
        case JSBOX_TOK_EQ_EQ:         return "==";
        case JSBOX_TOK_EQ_EQ_EQ:      return "===";
        case JSBOX_TOK_BANG_EQ:       return "!=";
        case JSBOX_TOK_BANG_EQ_EQ:    return "!==";
        case JSBOX_TOK_LT:            return "<";
        case JSBOX_TOK_GT:            return ">";
        case JSBOX_TOK_LT_EQ:         return "<=";
        case JSBOX_TOK_GT_EQ:         return ">=";
        case JSBOX_TOK_AMP:           return "&";
        case JSBOX_TOK_PIPE:          return "|";
        case JSBOX_TOK_CARET:         return "^";
        case JSBOX_TOK_TILDE:         return "~";
        case JSBOX_TOK_LT_LT:         return "<<";
        case JSBOX_TOK_GT_GT:         return ">>";
        case JSBOX_TOK_GT_GT_GT:      return ">>>";
        case JSBOX_TOK_AMP_AMP:       return "&&";
        case JSBOX_TOK_PIPE_PIPE:     return "||";
        case JSBOX_TOK_BANG:          return "!";
        case JSBOX_TOK_QUESTION:      return "?";
        case JSBOX_TOK_QUESTION_QUESTION: return "??";
        case JSBOX_TOK_QUESTION_DOT:  return "?.";
        case JSBOX_TOK_LPAREN:        return "(";
        case JSBOX_TOK_RPAREN:        return ")";
        case JSBOX_TOK_LBRACE:        return "{";
        case JSBOX_TOK_RBRACE:        return "}";
        case JSBOX_TOK_LBRACKET:      return "[";
        case JSBOX_TOK_RBRACKET:      return "]";
        case JSBOX_TOK_COMMA:         return ",";
        case JSBOX_TOK_DOT:           return ".";
        case JSBOX_TOK_SEMICOLON:     return ";";
        case JSBOX_TOK_COLON:         return ":";
        case JSBOX_TOK_ARROW:         return "=>";
        case JSBOX_TOK_DOTDOTDOT:     return "...";
        default:                      return "UNKNOWN";
    }
}

void jsbox_token_text(const JSBox_Token* token, char* buffer, size_t buffer_size) {
    size_t len = token->length;
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }
    memcpy(buffer, token->start, len);
    buffer[len] = '\0';
}

void jsbox_token_free(JSBox_Token* token) {
    if (token->string_value) {
        jsbox_free(token->string_value);
        token->string_value = NULL;
    }
}
