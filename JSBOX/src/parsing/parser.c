/*
 * JSBOX - JavaScript Engine
 *
 * Parsing: Pratt Parser Implementation
 *
 * Pratt parsing for expressions with proper operator precedence.
 * Recursive descent for statements.
 */

#include "parser.h"
#include "../base/memory.h"
#include "../base/strings.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Precedence Levels (Pratt Parser)
 * ============================================================================
 */

typedef enum {
  PREC_NONE,
  PREC_COMMA,      /* , */
  PREC_ASSIGNMENT, /* = += -= etc */
  PREC_TERNARY,    /* ?: */
  PREC_NULLISH,    /* ?? */
  PREC_OR,         /* || */
  PREC_AND,        /* && */
  PREC_BIT_OR,     /* | */
  PREC_BIT_XOR,    /* ^ */
  PREC_BIT_AND,    /* & */
  PREC_EQUALITY,   /* == != === !== */
  PREC_COMPARISON, /* < > <= >= in instanceof */
  PREC_SHIFT,      /* << >> >>> */
  PREC_TERM,       /* + - */
  PREC_FACTOR,     /* * / % */
  PREC_EXPONENT,   /* ** */
  PREC_UNARY,      /* ! - + ~ typeof void delete */
  PREC_UPDATE,     /* ++ -- */
  PREC_CALL,       /* () [] . */
  PREC_PRIMARY
} Precedence;

/* Forward declarations */
static JSBox_ASTNode *parse_expression(JSBox_Parser *parser);
static JSBox_ASTNode *parse_expression_prec(JSBox_Parser *parser,
                                            Precedence prec);
static JSBox_ASTNode *parse_statement(JSBox_Parser *parser);
static JSBox_ASTNode *parse_block(JSBox_Parser *parser);

/* ============================================================================
 * Parser Helpers
 * ============================================================================
 */

static void advance(JSBox_Parser *parser) {
  parser->previous = parser->current;

  for (;;) {
    parser->current = jsbox_lexer_next(parser->lexer);
    if (parser->current.type != JSBOX_TOK_ERROR)
      break;
    /* Errors are already reported by lexer */
    parser->had_error = true;
  }
}

static bool check(JSBox_Parser *parser, JSBox_TokenType type) {
  return parser->current.type == type;
}

static bool match(JSBox_Parser *parser, JSBox_TokenType type) {
  if (!check(parser, type))
    return false;
  advance(parser);
  return true;
}

static void error_at(JSBox_Parser *parser, JSBox_Token *token,
                     JSBox_ErrorCode code, const char *message) {
  if (parser->panic_mode)
    return;
  parser->panic_mode = true;
  parser->had_error = true;

  jsbox_diag_list_error(parser->diagnostics, code, message, token->span);
}

static void error(JSBox_Parser *parser, JSBox_ErrorCode code,
                  const char *message) {
  error_at(parser, &parser->previous, code, message);
}

static void error_current(JSBox_Parser *parser, JSBox_ErrorCode code,
                          const char *message) {
  error_at(parser, &parser->current, code, message);
}

static void consume(JSBox_Parser *parser, JSBox_TokenType type,
                    JSBox_ErrorCode code, const char *message) {
  if (parser->current.type == type) {
    advance(parser);
    return;
  }
  error_current(parser, code, message);
}

static void synchronize(JSBox_Parser *parser) {
  parser->panic_mode = false;

  while (parser->current.type != JSBOX_TOK_EOF) {
    if (parser->previous.type == JSBOX_TOK_SEMICOLON)
      return;

    switch (parser->current.type) {
    case JSBOX_TOK_FUNCTION:
    case JSBOX_TOK_VAR:
    case JSBOX_TOK_LET:
    case JSBOX_TOK_CONST:
    case JSBOX_TOK_IF:
    case JSBOX_TOK_WHILE:
    case JSBOX_TOK_FOR:
    case JSBOX_TOK_RETURN:
    case JSBOX_TOK_TRY:
    case JSBOX_TOK_THROW:
      return;
    default:
      break;
    }

    advance(parser);
  }
}

static JSBox_SourceSpan span_from_tokens(JSBox_Token start, JSBox_Token end) {
  JSBox_SourceSpan span;
  span.start = start.span.start;
  span.end = end.span.end;
  return span;
}

/* ============================================================================
 * Pratt Parser - Prefix Rules
 * ============================================================================
 */

static JSBox_ASTNode *parse_number(JSBox_Parser *parser) {
  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_NUMBER_LITERAL, parser->previous.span);
  node->as.number.value = parser->previous.number_value;
  return node;
}

static JSBox_ASTNode *parse_string(JSBox_Parser *parser) {
  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_STRING_LITERAL, parser->previous.span);
  node->as.string.value = parser->previous.string_value;
  parser->previous.string_value = NULL; /* Transfer ownership */
  return node;
}

static JSBox_ASTNode *parse_literal(JSBox_Parser *parser) {
  switch (parser->previous.type) {
  case JSBOX_TOK_TRUE: {
    JSBox_ASTNode *node =
        jsbox_ast_alloc(JSBOX_AST_BOOL_LITERAL, parser->previous.span);
    node->as.boolean.value = true;
    return node;
  }
  case JSBOX_TOK_FALSE: {
    JSBox_ASTNode *node =
        jsbox_ast_alloc(JSBOX_AST_BOOL_LITERAL, parser->previous.span);
    node->as.boolean.value = false;
    return node;
  }
  case JSBOX_TOK_NULL:
    return jsbox_ast_alloc(JSBOX_AST_NULL_LITERAL, parser->previous.span);
  case JSBOX_TOK_UNDEFINED:
    return jsbox_ast_alloc(JSBOX_AST_UNDEFINED_LITERAL, parser->previous.span);
  case JSBOX_TOK_THIS:
    return jsbox_ast_alloc(JSBOX_AST_THIS_EXPR, parser->previous.span);
  default:
    return NULL;
  }
}

static JSBox_ASTNode *parse_identifier(JSBox_Parser *parser) {
  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_IDENTIFIER, parser->previous.span);
  node->as.identifier.name =
      jsbox_strndup(parser->previous.start, parser->previous.length);
  return node;
}

static JSBox_ASTNode *parse_grouping(JSBox_Parser *parser) {
  JSBox_ASTNode *expr = parse_expression(parser);
  consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected ')' after expression");
  return expr;
}

static JSBox_ASTNode *parse_array(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;
  JSBox_ASTNode *node = jsbox_ast_alloc(JSBOX_AST_ARRAY_LITERAL, start.span);
  node->as.array.elements = NULL;

  if (!check(parser, JSBOX_TOK_RBRACKET)) {
    do {
      if (check(parser, JSBOX_TOK_RBRACKET))
        break; /* Allow trailing comma */

      JSBox_ASTNode *element = parse_expression_prec(parser, PREC_ASSIGNMENT);
      jsbox_ast_list_append(&node->as.array.elements, element);
    } while (match(parser, JSBOX_TOK_COMMA));
  }

  consume(parser, JSBOX_TOK_RBRACKET, JSBOX_ERR_EXPECTED_RBRACKET,
          "Expected ']' after array elements");
  node->span = span_from_tokens(start, parser->previous);
  return node;
}

static JSBox_ASTNode *parse_object(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;
  JSBox_ASTNode *node = jsbox_ast_alloc(JSBOX_AST_OBJECT_LITERAL, start.span);
  node->as.object.properties = NULL;

  if (!check(parser, JSBOX_TOK_RBRACE)) {
    do {
      if (check(parser, JSBOX_TOK_RBRACE))
        break; /* Allow trailing comma */

      JSBox_ASTNode *prop =
          jsbox_ast_alloc(JSBOX_AST_PROPERTY, parser->current.span);

      /* Key can be identifier, string, or number */
      if (check(parser, JSBOX_TOK_IDENTIFIER)) {
        advance(parser);
        prop->as.property.key = parse_identifier(parser);

        /* Shorthand: { x } instead of { x: x } */
        if (!check(parser, JSBOX_TOK_COLON)) {
          prop->as.property.shorthand = true;
          JSBox_ASTNode *value =
              jsbox_ast_alloc(JSBOX_AST_IDENTIFIER, parser->previous.span);
          value->as.identifier.name =
              jsbox_strdup(prop->as.property.key->as.identifier.name);
          prop->as.property.value = value;
          jsbox_ast_list_append(&node->as.object.properties, prop);
          continue;
        }
      } else if (check(parser, JSBOX_TOK_STRING)) {
        advance(parser);
        prop->as.property.key = parse_string(parser);
      } else if (check(parser, JSBOX_TOK_NUMBER)) {
        advance(parser);
        prop->as.property.key = parse_number(parser);
      } else if (check(parser, JSBOX_TOK_LBRACKET)) {
        /* Computed property: { [expr]: value } */
        advance(parser);
        prop->as.property.computed = true;
        prop->as.property.key = parse_expression(parser);
        consume(parser, JSBOX_TOK_RBRACKET, JSBOX_ERR_EXPECTED_RBRACKET,
                "Expected ']' after computed property key");
      } else {
        error_current(parser, JSBOX_ERR_UNEXPECTED_TOKEN,
                      "Expected property name");
        jsbox_ast_free(prop);
        break;
      }

      consume(parser, JSBOX_TOK_COLON, JSBOX_ERR_EXPECTED_COLON,
              "Expected ':' after property key");
      prop->as.property.value = parse_expression_prec(parser, PREC_ASSIGNMENT);

      jsbox_ast_list_append(&node->as.object.properties, prop);
    } while (match(parser, JSBOX_TOK_COMMA));
  }

  consume(parser, JSBOX_TOK_RBRACE, JSBOX_ERR_EXPECTED_RBRACE,
          "Expected '}' after object literal");
  node->span = span_from_tokens(start, parser->previous);
  return node;
}

static JSBox_ASTNode *parse_unary(JSBox_Parser *parser) {
  JSBox_Token op = parser->previous;
  JSBox_UnaryOp unary_op;

  switch (op.type) {
  case JSBOX_TOK_MINUS:
    unary_op = JSBOX_UNARY_NEG;
    break;
  case JSBOX_TOK_PLUS:
    unary_op = JSBOX_UNARY_POS;
    break;
  case JSBOX_TOK_BANG:
    unary_op = JSBOX_UNARY_NOT;
    break;
  case JSBOX_TOK_TILDE:
    unary_op = JSBOX_UNARY_BIT_NOT;
    break;
  case JSBOX_TOK_TYPEOF:
    unary_op = JSBOX_UNARY_TYPEOF;
    break;
  case JSBOX_TOK_VOID:
    unary_op = JSBOX_UNARY_VOID;
    break;
  case JSBOX_TOK_DELETE:
    unary_op = JSBOX_UNARY_DELETE;
    break;
  default:
    return NULL;
  }

  JSBox_ASTNode *argument = parse_expression_prec(parser, PREC_UNARY);

  JSBox_ASTNode *node = jsbox_ast_alloc(JSBOX_AST_UNARY_EXPR,
                                        span_from_tokens(op, parser->previous));
  node->as.unary.op = unary_op;
  node->as.unary.argument = argument;
  return node;
}

static JSBox_ASTNode *parse_prefix_update(JSBox_Parser *parser) {
  JSBox_Token op = parser->previous;
  bool increment = (op.type == JSBOX_TOK_PLUS_PLUS);

  JSBox_ASTNode *argument = parse_expression_prec(parser, PREC_UNARY);

  JSBox_ASTNode *node = jsbox_ast_alloc(JSBOX_AST_UPDATE_EXPR,
                                        span_from_tokens(op, parser->previous));
  node->as.update.increment = increment;
  node->as.update.prefix = true;
  node->as.update.argument = argument;
  return node;
}

static JSBox_ASTNode *parse_function(JSBox_Parser *parser, bool is_expression);

static JSBox_ASTNode *parse_function_expr(JSBox_Parser *parser) {
  return parse_function(parser, true);
}

static JSBox_ASTNode *parse_new(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;

  JSBox_ASTNode *callee = parse_expression_prec(parser, PREC_CALL);

  JSBox_ASTNodeList *arguments = NULL;
  if (match(parser, JSBOX_TOK_LPAREN)) {
    if (!check(parser, JSBOX_TOK_RPAREN)) {
      do {
        JSBox_ASTNode *arg = parse_expression_prec(parser, PREC_ASSIGNMENT);
        jsbox_ast_list_append(&arguments, arg);
      } while (match(parser, JSBOX_TOK_COMMA));
    }
    consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
            "Expected ')' after arguments");
  }

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_NEW_EXPR, span_from_tokens(start, parser->previous));
  node->as.new_expr.callee = callee;
  node->as.new_expr.arguments = arguments;
  return node;
}

/* ============================================================================
 * Pratt Parser - Infix Rules
 * ============================================================================
 */

static JSBox_ASTNode *parse_binary(JSBox_Parser *parser, JSBox_ASTNode *left,
                                   Precedence prec) {
  JSBox_Token op = parser->previous;
  JSBox_BinaryOp binary_op;

  switch (op.type) {
  case JSBOX_TOK_PLUS:
    binary_op = JSBOX_OP_ADD;
    break;
  case JSBOX_TOK_MINUS:
    binary_op = JSBOX_OP_SUB;
    break;
  case JSBOX_TOK_STAR:
    binary_op = JSBOX_OP_MUL;
    break;
  case JSBOX_TOK_SLASH:
    binary_op = JSBOX_OP_DIV;
    break;
  case JSBOX_TOK_PERCENT:
    binary_op = JSBOX_OP_MOD;
    break;
  case JSBOX_TOK_STAR_STAR:
    binary_op = JSBOX_OP_POW;
    break;
  case JSBOX_TOK_EQ_EQ:
    binary_op = JSBOX_OP_EQ;
    break;
  case JSBOX_TOK_EQ_EQ_EQ:
    binary_op = JSBOX_OP_STRICT_EQ;
    break;
  case JSBOX_TOK_BANG_EQ:
    binary_op = JSBOX_OP_NE;
    break;
  case JSBOX_TOK_BANG_EQ_EQ:
    binary_op = JSBOX_OP_STRICT_NE;
    break;
  case JSBOX_TOK_LT:
    binary_op = JSBOX_OP_LT;
    break;
  case JSBOX_TOK_GT:
    binary_op = JSBOX_OP_GT;
    break;
  case JSBOX_TOK_LT_EQ:
    binary_op = JSBOX_OP_LE;
    break;
  case JSBOX_TOK_GT_EQ:
    binary_op = JSBOX_OP_GE;
    break;
  case JSBOX_TOK_AMP_AMP:
    binary_op = JSBOX_OP_AND;
    break;
  case JSBOX_TOK_PIPE_PIPE:
    binary_op = JSBOX_OP_OR;
    break;
  case JSBOX_TOK_QUESTION_QUESTION:
    binary_op = JSBOX_OP_NULLISH;
    break;
  case JSBOX_TOK_AMP:
    binary_op = JSBOX_OP_BIT_AND;
    break;
  case JSBOX_TOK_PIPE:
    binary_op = JSBOX_OP_BIT_OR;
    break;
  case JSBOX_TOK_CARET:
    binary_op = JSBOX_OP_BIT_XOR;
    break;
  case JSBOX_TOK_LT_LT:
    binary_op = JSBOX_OP_SHL;
    break;
  case JSBOX_TOK_GT_GT:
    binary_op = JSBOX_OP_SHR;
    break;
  case JSBOX_TOK_GT_GT_GT:
    binary_op = JSBOX_OP_USHR;
    break;
  case JSBOX_TOK_IN:
    binary_op = JSBOX_OP_IN;
    break;
  case JSBOX_TOK_INSTANCEOF:
    binary_op = JSBOX_OP_INSTANCEOF;
    break;
  default:
    return left;
  }

  /* Right-associative for ** */
  Precedence next_prec = (op.type == JSBOX_TOK_STAR_STAR) ? prec : prec + 1;
  JSBox_ASTNode *right = parse_expression_prec(parser, next_prec);

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_BINARY_EXPR,
      span_from_tokens(
          left->span.start.offset == 0 ? op : (JSBox_Token){.span = left->span},
          parser->previous));
  node->span.start = left->span.start;
  node->as.binary.op = binary_op;
  node->as.binary.left = left;
  node->as.binary.right = right;
  return node;
}

static JSBox_ASTNode *parse_assignment(JSBox_Parser *parser,
                                       JSBox_ASTNode *left, Precedence prec) {
  (void)prec;
  JSBox_Token op = parser->previous;
  JSBox_AssignOp assign_op;

  switch (op.type) {
  case JSBOX_TOK_EQ:
    assign_op = JSBOX_ASSIGN;
    break;
  case JSBOX_TOK_PLUS_EQ:
    assign_op = JSBOX_ASSIGN_ADD;
    break;
  case JSBOX_TOK_MINUS_EQ:
    assign_op = JSBOX_ASSIGN_SUB;
    break;
  case JSBOX_TOK_STAR_EQ:
    assign_op = JSBOX_ASSIGN_MUL;
    break;
  case JSBOX_TOK_SLASH_EQ:
    assign_op = JSBOX_ASSIGN_DIV;
    break;
  case JSBOX_TOK_PERCENT_EQ:
    assign_op = JSBOX_ASSIGN_MOD;
    break;
  default:
    return left;
  }

  /* Check left is valid assignment target */
  if (left->type != JSBOX_AST_IDENTIFIER &&
      left->type != JSBOX_AST_MEMBER_EXPR) {
    error(parser, JSBOX_ERR_INVALID_ASSIGNMENT, "Invalid assignment target");
  }

  JSBox_ASTNode *right = parse_expression_prec(parser, PREC_ASSIGNMENT);

  JSBox_ASTNode *node = jsbox_ast_alloc(JSBOX_AST_ASSIGNMENT_EXPR,
                                        span_from_tokens(op, parser->previous));
  node->span.start = left->span.start;
  node->as.assignment.op = assign_op;
  node->as.assignment.left = left;
  node->as.assignment.right = right;
  return node;
}

static JSBox_ASTNode *parse_ternary(JSBox_Parser *parser, JSBox_ASTNode *left,
                                    Precedence prec) {
  (void)prec;
  JSBox_ASTNode *consequent = parse_expression_prec(parser, PREC_ASSIGNMENT);
  consume(parser, JSBOX_TOK_COLON, JSBOX_ERR_EXPECTED_COLON,
          "Expected ':' in ternary expression");
  JSBox_ASTNode *alternate = parse_expression_prec(parser, PREC_ASSIGNMENT);

  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_CONDITIONAL_EXPR,
                      span_from_tokens(parser->previous, parser->previous));
  node->span.start = left->span.start;
  node->as.conditional.test = left;
  node->as.conditional.consequent = consequent;
  node->as.conditional.alternate = alternate;
  return node;
}

static JSBox_ASTNode *parse_call(JSBox_Parser *parser, JSBox_ASTNode *left,
                                 Precedence prec) {
  (void)prec;
  JSBox_ASTNodeList *arguments = NULL;

  if (!check(parser, JSBOX_TOK_RPAREN)) {
    do {
      JSBox_ASTNode *arg = parse_expression_prec(parser, PREC_ASSIGNMENT);
      jsbox_ast_list_append(&arguments, arg);
    } while (match(parser, JSBOX_TOK_COMMA));
  }

  consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected ')' after arguments");

  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_CALL_EXPR,
                      span_from_tokens(parser->previous, parser->previous));
  node->span.start = left->span.start;
  node->as.call.callee = left;
  node->as.call.arguments = arguments;
  return node;
}

static JSBox_ASTNode *parse_member(JSBox_Parser *parser, JSBox_ASTNode *left,
                                   Precedence prec) {
  (void)prec;
  bool computed = (parser->previous.type == JSBOX_TOK_LBRACKET);
  JSBox_ASTNode *property;

  if (computed) {
    property = parse_expression(parser);
    consume(parser, JSBOX_TOK_RBRACKET, JSBOX_ERR_EXPECTED_RBRACKET,
            "Expected ']' after computed property");
  } else {
    consume(parser, JSBOX_TOK_IDENTIFIER, JSBOX_ERR_EXPECTED_IDENTIFIER,
            "Expected property name after '.'");
    property = parse_identifier(parser);
  }

  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_MEMBER_EXPR,
                      span_from_tokens(parser->previous, parser->previous));
  node->span.start = left->span.start;
  node->as.member.object = left;
  node->as.member.property = property;
  node->as.member.computed = computed;
  return node;
}

static JSBox_ASTNode *parse_postfix_update(JSBox_Parser *parser,
                                           JSBox_ASTNode *left,
                                           Precedence prec) {
  (void)prec;
  bool increment = (parser->previous.type == JSBOX_TOK_PLUS_PLUS);

  JSBox_ASTNode *node =
      jsbox_ast_alloc(JSBOX_AST_UPDATE_EXPR,
                      span_from_tokens(parser->previous, parser->previous));
  node->span.start = left->span.start;
  node->as.update.increment = increment;
  node->as.update.prefix = false;
  node->as.update.argument = left;
  return node;
}

/* ============================================================================
 * Pratt Parser - Rule Table
 * ============================================================================
 */

typedef JSBox_ASTNode *(*PrefixFn)(JSBox_Parser *);
typedef JSBox_ASTNode *(*InfixFn)(JSBox_Parser *, JSBox_ASTNode *, Precedence);

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence precedence;
} ParseRule;

static ParseRule rules[JSBOX_TOK_COUNT];
static bool rules_initialized = false;

static void init_rules(void) {
  if (rules_initialized)
    return;

  /* Initialize all to NULL/NONE first */
  for (int i = 0; i < JSBOX_TOK_COUNT; i++) {
    rules[i] = (ParseRule){NULL, NULL, PREC_NONE};
  }

  /* Literals */
  rules[JSBOX_TOK_NUMBER] = (ParseRule){parse_number, NULL, PREC_NONE};
  rules[JSBOX_TOK_STRING] = (ParseRule){parse_string, NULL, PREC_NONE};
  rules[JSBOX_TOK_IDENTIFIER] = (ParseRule){parse_identifier, NULL, PREC_NONE};
  rules[JSBOX_TOK_TRUE] = (ParseRule){parse_literal, NULL, PREC_NONE};
  rules[JSBOX_TOK_FALSE] = (ParseRule){parse_literal, NULL, PREC_NONE};
  rules[JSBOX_TOK_NULL] = (ParseRule){parse_literal, NULL, PREC_NONE};
  rules[JSBOX_TOK_UNDEFINED] = (ParseRule){parse_literal, NULL, PREC_NONE};
  rules[JSBOX_TOK_THIS] = (ParseRule){parse_literal, NULL, PREC_NONE};

  /* Grouping and collections */
  rules[JSBOX_TOK_LPAREN] = (ParseRule){parse_grouping, parse_call, PREC_CALL};
  rules[JSBOX_TOK_LBRACKET] = (ParseRule){parse_array, parse_member, PREC_CALL};
  rules[JSBOX_TOK_LBRACE] = (ParseRule){parse_object, NULL, PREC_NONE};

  /* Operators - prefix */
  rules[JSBOX_TOK_MINUS] = (ParseRule){parse_unary, parse_binary, PREC_TERM};
  rules[JSBOX_TOK_PLUS] = (ParseRule){parse_unary, parse_binary, PREC_TERM};
  rules[JSBOX_TOK_BANG] = (ParseRule){parse_unary, NULL, PREC_NONE};
  rules[JSBOX_TOK_TILDE] = (ParseRule){parse_unary, NULL, PREC_NONE};
  rules[JSBOX_TOK_TYPEOF] = (ParseRule){parse_unary, NULL, PREC_NONE};
  rules[JSBOX_TOK_VOID] = (ParseRule){parse_unary, NULL, PREC_NONE};
  rules[JSBOX_TOK_DELETE] = (ParseRule){parse_unary, NULL, PREC_NONE};
  rules[JSBOX_TOK_PLUS_PLUS] =
      (ParseRule){parse_prefix_update, parse_postfix_update, PREC_UPDATE};
  rules[JSBOX_TOK_MINUS_MINUS] =
      (ParseRule){parse_prefix_update, parse_postfix_update, PREC_UPDATE};
  rules[JSBOX_TOK_NEW] = (ParseRule){parse_new, NULL, PREC_NONE};
  rules[JSBOX_TOK_FUNCTION] = (ParseRule){parse_function_expr, NULL, PREC_NONE};

  /* Binary operators */
  rules[JSBOX_TOK_STAR] = (ParseRule){NULL, parse_binary, PREC_FACTOR};
  rules[JSBOX_TOK_SLASH] = (ParseRule){NULL, parse_binary, PREC_FACTOR};
  rules[JSBOX_TOK_PERCENT] = (ParseRule){NULL, parse_binary, PREC_FACTOR};
  rules[JSBOX_TOK_STAR_STAR] = (ParseRule){NULL, parse_binary, PREC_EXPONENT};

  rules[JSBOX_TOK_EQ_EQ] = (ParseRule){NULL, parse_binary, PREC_EQUALITY};
  rules[JSBOX_TOK_EQ_EQ_EQ] = (ParseRule){NULL, parse_binary, PREC_EQUALITY};
  rules[JSBOX_TOK_BANG_EQ] = (ParseRule){NULL, parse_binary, PREC_EQUALITY};
  rules[JSBOX_TOK_BANG_EQ_EQ] = (ParseRule){NULL, parse_binary, PREC_EQUALITY};

  rules[JSBOX_TOK_LT] = (ParseRule){NULL, parse_binary, PREC_COMPARISON};
  rules[JSBOX_TOK_GT] = (ParseRule){NULL, parse_binary, PREC_COMPARISON};
  rules[JSBOX_TOK_LT_EQ] = (ParseRule){NULL, parse_binary, PREC_COMPARISON};
  rules[JSBOX_TOK_GT_EQ] = (ParseRule){NULL, parse_binary, PREC_COMPARISON};
  rules[JSBOX_TOK_IN] = (ParseRule){NULL, parse_binary, PREC_COMPARISON};
  rules[JSBOX_TOK_INSTANCEOF] =
      (ParseRule){NULL, parse_binary, PREC_COMPARISON};

  rules[JSBOX_TOK_LT_LT] = (ParseRule){NULL, parse_binary, PREC_SHIFT};
  rules[JSBOX_TOK_GT_GT] = (ParseRule){NULL, parse_binary, PREC_SHIFT};
  rules[JSBOX_TOK_GT_GT_GT] = (ParseRule){NULL, parse_binary, PREC_SHIFT};

  rules[JSBOX_TOK_AMP] = (ParseRule){NULL, parse_binary, PREC_BIT_AND};
  rules[JSBOX_TOK_PIPE] = (ParseRule){NULL, parse_binary, PREC_BIT_OR};
  rules[JSBOX_TOK_CARET] = (ParseRule){NULL, parse_binary, PREC_BIT_XOR};

  rules[JSBOX_TOK_AMP_AMP] = (ParseRule){NULL, parse_binary, PREC_AND};
  rules[JSBOX_TOK_PIPE_PIPE] = (ParseRule){NULL, parse_binary, PREC_OR};
  rules[JSBOX_TOK_QUESTION_QUESTION] =
      (ParseRule){NULL, parse_binary, PREC_NULLISH};

  /* Assignment */
  rules[JSBOX_TOK_EQ] = (ParseRule){NULL, parse_assignment, PREC_ASSIGNMENT};
  rules[JSBOX_TOK_PLUS_EQ] =
      (ParseRule){NULL, parse_assignment, PREC_ASSIGNMENT};
  rules[JSBOX_TOK_MINUS_EQ] =
      (ParseRule){NULL, parse_assignment, PREC_ASSIGNMENT};
  rules[JSBOX_TOK_STAR_EQ] =
      (ParseRule){NULL, parse_assignment, PREC_ASSIGNMENT};
  rules[JSBOX_TOK_SLASH_EQ] =
      (ParseRule){NULL, parse_assignment, PREC_ASSIGNMENT};
  rules[JSBOX_TOK_PERCENT_EQ] =
      (ParseRule){NULL, parse_assignment, PREC_ASSIGNMENT};

  /* Ternary */
  rules[JSBOX_TOK_QUESTION] = (ParseRule){NULL, parse_ternary, PREC_TERNARY};

  /* Member access */
  rules[JSBOX_TOK_DOT] = (ParseRule){NULL, parse_member, PREC_CALL};

  rules_initialized = true;
}

static ParseRule *get_rule(JSBox_TokenType type) { return &rules[type]; }

/* ============================================================================
 * Expression Parsing
 * ============================================================================
 */

static JSBox_ASTNode *parse_expression_prec(JSBox_Parser *parser,
                                            Precedence prec) {
  advance(parser);

  PrefixFn prefix = get_rule(parser->previous.type)->prefix;
  if (!prefix) {
    error(parser, JSBOX_ERR_EXPECTED_EXPRESSION, "Expected expression");
    return NULL;
  }

  JSBox_ASTNode *left = prefix(parser);

  while (prec <= get_rule(parser->current.type)->precedence) {
    advance(parser);
    InfixFn infix = get_rule(parser->previous.type)->infix;
    if (infix) {
      left = infix(parser, left, get_rule(parser->previous.type)->precedence);
    }
  }

  return left;
}

static JSBox_ASTNode *parse_expression(JSBox_Parser *parser) {
  return parse_expression_prec(parser, PREC_ASSIGNMENT);
}

/* ============================================================================
 * Statement Parsing
 * ============================================================================
 */

static JSBox_ASTNode *parse_var_decl(JSBox_Parser *parser, JSBox_VarKind kind) {
  JSBox_Token start = parser->previous;

  consume(parser, JSBOX_TOK_IDENTIFIER, JSBOX_ERR_EXPECTED_IDENTIFIER,
          "Expected variable name");

  char *name = jsbox_strndup(parser->previous.start, parser->previous.length);

  JSBox_ASTNode *init = NULL;
  if (match(parser, JSBOX_TOK_EQ)) {
    init = parse_expression(parser);
  }

  consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
          "Expected ';' after variable declaration");

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_VAR_DECL, span_from_tokens(start, parser->previous));
  node->as.var_decl.kind = kind;
  node->as.var_decl.name = name;
  node->as.var_decl.init = init;
  return node;
}

static JSBox_ASTNode *parse_block(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;
  JSBox_ASTNode *node = jsbox_ast_alloc(JSBOX_AST_BLOCK_STMT, start.span);
  node->as.program.body = NULL;

  while (!check(parser, JSBOX_TOK_RBRACE) && !check(parser, JSBOX_TOK_EOF)) {
    JSBox_ASTNode *stmt = parse_statement(parser);
    if (stmt) {
      jsbox_ast_list_append(&node->as.program.body, stmt);
    }
  }

  consume(parser, JSBOX_TOK_RBRACE, JSBOX_ERR_EXPECTED_RBRACE,
          "Expected '}' after block");
  node->span = span_from_tokens(start, parser->previous);
  return node;
}

static JSBox_ASTNode *parse_if_stmt(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;

  consume(parser, JSBOX_TOK_LPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected '(' after 'if'");
  JSBox_ASTNode *test = parse_expression(parser);
  consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected ')' after condition");

  JSBox_ASTNode *consequent = parse_statement(parser);
  JSBox_ASTNode *alternate = NULL;

  if (match(parser, JSBOX_TOK_ELSE)) {
    alternate = parse_statement(parser);
  }

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_IF_STMT, span_from_tokens(start, parser->previous));
  node->as.if_stmt.test = test;
  node->as.if_stmt.consequent = consequent;
  node->as.if_stmt.alternate = alternate;
  return node;
}

static JSBox_ASTNode *parse_while_stmt(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;

  consume(parser, JSBOX_TOK_LPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected '(' after 'while'");
  JSBox_ASTNode *test = parse_expression(parser);
  consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected ')' after condition");

  JSBox_ASTNode *body = parse_statement(parser);

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_WHILE_STMT, span_from_tokens(start, parser->previous));
  node->as.while_stmt.test = test;
  node->as.while_stmt.body = body;
  return node;
}

static JSBox_ASTNode *parse_for_stmt(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;

  consume(parser, JSBOX_TOK_LPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected '(' after 'for'");

  /* Init */
  JSBox_ASTNode *init = NULL;
  if (match(parser, JSBOX_TOK_SEMICOLON)) {
    /* No init */
  } else if (match(parser, JSBOX_TOK_VAR)) {
    init = parse_var_decl(parser, JSBOX_VAR_VAR);
  } else if (match(parser, JSBOX_TOK_LET)) {
    init = parse_var_decl(parser, JSBOX_VAR_LET);
  } else {
    init = parse_expression(parser);
    consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
            "Expected ';'");
  }

  /* Test */
  JSBox_ASTNode *test = NULL;
  if (!check(parser, JSBOX_TOK_SEMICOLON)) {
    test = parse_expression(parser);
  }
  consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
          "Expected ';'");

  /* Update */
  JSBox_ASTNode *update = NULL;
  if (!check(parser, JSBOX_TOK_RPAREN)) {
    update = parse_expression(parser);
  }
  consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected ')' after for clauses");

  JSBox_ASTNode *body = parse_statement(parser);

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_FOR_STMT, span_from_tokens(start, parser->previous));
  node->as.for_stmt.init = init;
  node->as.for_stmt.test = test;
  node->as.for_stmt.update = update;
  node->as.for_stmt.body = body;
  return node;
}

static JSBox_ASTNode *parse_return_stmt(JSBox_Parser *parser) {
  JSBox_Token start = parser->previous;
  JSBox_ASTNode *argument = NULL;

  if (!check(parser, JSBOX_TOK_SEMICOLON) && !check(parser, JSBOX_TOK_RBRACE)) {
    argument = parse_expression(parser);
  }

  consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
          "Expected ';' after return");

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_RETURN_STMT, span_from_tokens(start, parser->previous));
  node->as.return_stmt.argument = argument;
  return node;
}

static JSBox_ASTNode *parse_function(JSBox_Parser *parser, bool is_expression) {
  JSBox_Token start = parser->previous;
  char *name = NULL;

  if (check(parser, JSBOX_TOK_IDENTIFIER)) {
    advance(parser);
    name = jsbox_strndup(parser->previous.start, parser->previous.length);
  } else if (!is_expression) {
    error_current(parser, JSBOX_ERR_EXPECTED_IDENTIFIER,
                  "Expected function name");
  }

  consume(parser, JSBOX_TOK_LPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected '(' after function name");

  /* Parse parameters */
  JSBox_ASTNodeList *params = NULL;
  if (!check(parser, JSBOX_TOK_RPAREN)) {
    do {
      consume(parser, JSBOX_TOK_IDENTIFIER, JSBOX_ERR_EXPECTED_IDENTIFIER,
              "Expected parameter name");
      JSBox_ASTNode *param = parse_identifier(parser);
      jsbox_ast_list_append(&params, param);
    } while (match(parser, JSBOX_TOK_COMMA));
  }

  consume(parser, JSBOX_TOK_RPAREN, JSBOX_ERR_EXPECTED_RPAREN,
          "Expected ')' after parameters");
  consume(parser, JSBOX_TOK_LBRACE, JSBOX_ERR_EXPECTED_RBRACE,
          "Expected '{' before function body");

  JSBox_ASTNode *body = parse_block(parser);

  JSBox_ASTNodeType type =
      is_expression ? JSBOX_AST_FUNCTION_EXPR : JSBOX_AST_FUNCTION_DECL;
  JSBox_ASTNode *node =
      jsbox_ast_alloc(type, span_from_tokens(start, parser->previous));
  node->as.function.name = name;
  node->as.function.params = params;
  node->as.function.body = body;
  return node;
}

static JSBox_ASTNode *parse_statement(JSBox_Parser *parser) {
  if (parser->panic_mode) {
    synchronize(parser);
  }

  if (match(parser, JSBOX_TOK_VAR)) {
    return parse_var_decl(parser, JSBOX_VAR_VAR);
  }
  if (match(parser, JSBOX_TOK_LET)) {
    return parse_var_decl(parser, JSBOX_VAR_LET);
  }
  if (match(parser, JSBOX_TOK_CONST)) {
    return parse_var_decl(parser, JSBOX_VAR_CONST);
  }
  if (match(parser, JSBOX_TOK_FUNCTION)) {
    return parse_function(parser, false);
  }
  if (match(parser, JSBOX_TOK_LBRACE)) {
    return parse_block(parser);
  }
  if (match(parser, JSBOX_TOK_IF)) {
    return parse_if_stmt(parser);
  }
  if (match(parser, JSBOX_TOK_WHILE)) {
    return parse_while_stmt(parser);
  }
  if (match(parser, JSBOX_TOK_FOR)) {
    return parse_for_stmt(parser);
  }
  if (match(parser, JSBOX_TOK_RETURN)) {
    return parse_return_stmt(parser);
  }
  if (match(parser, JSBOX_TOK_BREAK)) {
    consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
            "Expected ';' after break");
    return jsbox_ast_alloc(JSBOX_AST_BREAK_STMT, parser->previous.span);
  }
  if (match(parser, JSBOX_TOK_CONTINUE)) {
    consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
            "Expected ';' after continue");
    return jsbox_ast_alloc(JSBOX_AST_CONTINUE_STMT, parser->previous.span);
  }
  if (match(parser, JSBOX_TOK_SEMICOLON)) {
    return jsbox_ast_alloc(JSBOX_AST_EMPTY_STMT, parser->previous.span);
  }

  /* Expression statement */
  JSBox_Token start = parser->current;
  JSBox_ASTNode *expr = parse_expression(parser);
  consume(parser, JSBOX_TOK_SEMICOLON, JSBOX_ERR_EXPECTED_SEMICOLON,
          "Expected ';' after expression");

  JSBox_ASTNode *node = jsbox_ast_alloc(
      JSBOX_AST_EXPR_STMT, span_from_tokens(start, parser->previous));
  node->as.expr_stmt.expression = expr;
  return node;
}

/* ============================================================================
 * Public API
 * ============================================================================
 */

JSBox_Parser *jsbox_parser_create(const char *source, const char *filename) {
  init_rules();

  JSBox_Parser *parser = (JSBox_Parser *)jsbox_malloc(sizeof(JSBox_Parser));
  parser->lexer = jsbox_lexer_create(source, filename);
  parser->diagnostics = jsbox_diag_list_create();
  parser->had_error = false;
  parser->panic_mode = false;

  /* Prime the parser */
  advance(parser);

  return parser;
}

void jsbox_parser_destroy(JSBox_Parser *parser) {
  if (parser) {
    jsbox_lexer_destroy(parser->lexer);
    jsbox_diag_list_destroy(parser->diagnostics);
    jsbox_free(parser);
  }
}

JSBox_ASTNode *jsbox_parser_parse(JSBox_Parser *parser) {
  JSBox_ASTNode *program =
      jsbox_ast_alloc(JSBOX_AST_PROGRAM, jsbox_span_empty());
  program->as.program.body = NULL;

  while (!check(parser, JSBOX_TOK_EOF)) {
    JSBox_ASTNode *stmt = parse_statement(parser);
    if (stmt) {
      jsbox_ast_list_append(&program->as.program.body, stmt);
    }
  }

  return program;
}

JSBox_ASTNode *jsbox_parser_parse_expression(JSBox_Parser *parser) {
  return parse_expression(parser);
}

JSBox_DiagnosticList *jsbox_parser_diagnostics(JSBox_Parser *parser) {
  return parser->diagnostics;
}

JSBox_SourceFile *jsbox_parser_source_file(JSBox_Parser *parser) {
  return jsbox_lexer_source_file(parser->lexer);
}

bool jsbox_parser_has_errors(const JSBox_Parser *parser) {
  return parser->had_error;
}
