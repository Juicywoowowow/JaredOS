#include "parser.hpp"
#include <stdexcept>
#include <iostream>

namespace cu {

Parser::Parser(Lexer& lexer) : lexer_(lexer) {
    advance();
}

void Parser::advance() {
    current_ = lexer_.next();
}

bool Parser::check(TokenType type) {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& msg) {
    if (!check(type)) {
        error(msg);
    }
    Token t = current_;
    advance();
    return t;
}

void Parser::error(const std::string& msg) {
    std::string full_msg = "Parse error: " + msg + "\n";
    
    // Add file and location info
    full_msg += "  --> " + current_.loc.file + ":" + 
                std::to_string(current_.loc.line) + ":" + 
                std::to_string(current_.loc.column) + "\n";
    
    // Get and show the source line
    std::string line = lexer_.get_line(current_.loc.line);
    if (!line.empty()) {
        // Line number with padding
        std::string line_num = std::to_string(current_.loc.line);
        full_msg += "   " + line_num + " | " + line + "\n";
        
        // Caret pointing to error position
        std::string padding(line_num.length(), ' ');
        std::string spaces(current_.loc.column > 0 ? current_.loc.column - 1 : 0, ' ');
        full_msg += "   " + padding + " | " + spaces + "^\n";
    }
    
    throw std::runtime_error(full_msg);
}

TranslationUnit Parser::parse() {
    TranslationUnit unit;
    unit.filename = current_.loc.file;
    
    while (!check(TokenType::END_OF_FILE)) {
        auto decl = parse_declaration();
        if (decl) unit.declarations.push_back(std::move(decl));
    }
    
    return unit;
}

std::vector<std::string> Parser::parse_attributes() {
    std::vector<std::string> attrs;
    while (match(TokenType::AT)) {
        Token name = expect(TokenType::IDENTIFIER, "Expected attribute name");
        std::string attr = name.value;
        if (match(TokenType::LPAREN)) {
            // Parse attribute argument
            attr += "(";
            attr += current_.value;
            advance();
            expect(TokenType::RPAREN, "Expected )");
            attr += ")";
        }
        attrs.push_back(attr);
    }
    return attrs;
}

DeclPtr Parser::parse_declaration() {
    auto attrs = parse_attributes();
    
    if (match(TokenType::IMPORT)) {
        return parse_import();
    }
    if (check(TokenType::FN)) {
        auto decl = parse_function();
        for (const auto& a : attrs) {
            if (a == "export") decl->is_export = true;
            else if (a == "nomangle") decl->is_nomangle = true;
            else if (a == "inline") decl->is_inline = true;
        }
        return decl;
    }
    if (check(TokenType::STRUCT)) {
        auto decl = parse_struct();
        for (const auto& a : attrs) {
            if (a == "packed") decl->is_packed = true;
            else if (a.find("align(") == 0) {
                decl->align = std::stoi(a.substr(6, a.size() - 7));
            }
        }
        return decl;
    }
    if (check(TokenType::UNION)) return parse_union();
    if (check(TokenType::ENUM)) return parse_enum();
    if (check(TokenType::TYPEDEF)) return parse_typedef();
    if (check(TokenType::EXTERN)) {
        advance();
        expect(TokenType::FN, "Expected fn after extern");
        auto decl = parse_function();
        decl->is_extern = true;
        return decl;
    }
    
    error("Expected declaration, got: " + current_.value);
    return nullptr;
}

DeclPtr Parser::parse_import() {
    expect(TokenType::FN, "Expected fn after import");
    
    auto decl = std::make_unique<Decl>();
    decl->kind = Decl::IMPORT;
    decl->is_import = true;
    decl->loc = current_.loc;
    
    decl->name = expect(TokenType::IDENTIFIER, "Expected function name").value;
    
    expect(TokenType::LPAREN, "Expected (");
    while (!check(TokenType::RPAREN)) {
        // Check for variadic ...
        if (match(TokenType::ELLIPSIS)) {
            decl->is_variadic = true;
            break;
        }
        Type param_type = parse_type();
        std::string param_name;
        if (check(TokenType::IDENTIFIER)) {
            param_name = current_.value;
            advance();
        }
        decl->params.push_back({param_name, std::move(param_type)});
        if (!check(TokenType::RPAREN) && !check(TokenType::ELLIPSIS)) expect(TokenType::COMMA, "Expected ,");
    }
    expect(TokenType::RPAREN, "Expected )");
    
    expect(TokenType::ARROW, "Expected ->");
    decl->return_type = std::make_unique<Type>(parse_type());
    
    expect(TokenType::SEMICOLON, "Expected ;");
    
    return decl;
}

DeclPtr Parser::parse_function() {
    auto decl = std::make_unique<Decl>();
    decl->kind = Decl::FUNCTION;
    decl->loc = current_.loc;
    
    expect(TokenType::FN, "Expected fn");
    decl->name = expect(TokenType::IDENTIFIER, "Expected function name").value;
    
    expect(TokenType::LPAREN, "Expected (");
    while (!check(TokenType::RPAREN)) {
        Type param_type = parse_type();
        std::string param_name = expect(TokenType::IDENTIFIER, "Expected parameter name").value;
        decl->params.push_back({param_name, std::move(param_type)});
        if (!check(TokenType::RPAREN)) expect(TokenType::COMMA, "Expected ,");
    }
    expect(TokenType::RPAREN, "Expected )");
    
    expect(TokenType::ARROW, "Expected ->");
    decl->return_type = std::make_unique<Type>(parse_type());
    
    if (check(TokenType::LBRACE)) {
        auto body = parse_block();
        decl->body = std::move(body->statements);
    } else {
        expect(TokenType::SEMICOLON, "Expected ; or {");
    }
    
    return decl;
}

DeclPtr Parser::parse_struct() {
    auto decl = std::make_unique<Decl>();
    decl->kind = Decl::STRUCT;
    decl->loc = current_.loc;
    
    expect(TokenType::STRUCT, "Expected struct");
    decl->name = expect(TokenType::IDENTIFIER, "Expected struct name").value;
    
    expect(TokenType::LBRACE, "Expected {");
    while (!check(TokenType::RBRACE)) {
        Type field_type = parse_type();
        std::string field_name = expect(TokenType::IDENTIFIER, "Expected field name").value;
        expect(TokenType::SEMICOLON, "Expected ;");
        decl->fields.push_back({field_name, std::move(field_type)});
    }
    expect(TokenType::RBRACE, "Expected }");
    
    return decl;
}

DeclPtr Parser::parse_union() {
    auto decl = std::make_unique<Decl>();
    decl->kind = Decl::UNION;
    decl->loc = current_.loc;
    
    expect(TokenType::UNION, "Expected union");
    decl->name = expect(TokenType::IDENTIFIER, "Expected union name").value;
    
    expect(TokenType::LBRACE, "Expected {");
    while (!check(TokenType::RBRACE)) {
        Type field_type = parse_type();
        std::string field_name = expect(TokenType::IDENTIFIER, "Expected field name").value;
        expect(TokenType::SEMICOLON, "Expected ;");
        decl->fields.push_back({field_name, std::move(field_type)});
    }
    expect(TokenType::RBRACE, "Expected }");
    
    return decl;
}

DeclPtr Parser::parse_enum() {
    auto decl = std::make_unique<Decl>();
    decl->kind = Decl::ENUM;
    decl->loc = current_.loc;
    
    expect(TokenType::ENUM, "Expected enum");
    decl->name = expect(TokenType::IDENTIFIER, "Expected enum name").value;
    
    expect(TokenType::LBRACE, "Expected {");
    while (!check(TokenType::RBRACE)) {
        std::string value_name = expect(TokenType::IDENTIFIER, "Expected enum value").value;
        std::optional<int64_t> value;
        if (match(TokenType::ASSIGN)) {
            bool neg = match(TokenType::MINUS);
            Token num = expect(TokenType::INTEGER, "Expected integer");
            int64_t v = std::stoll(num.value);
            value = neg ? -v : v;
        }
        decl->enum_values.push_back({value_name, value});
        if (!check(TokenType::RBRACE)) match(TokenType::COMMA);
    }
    expect(TokenType::RBRACE, "Expected }");
    
    return decl;
}

DeclPtr Parser::parse_typedef() {
    auto decl = std::make_unique<Decl>();
    decl->kind = Decl::TYPEDEF;
    decl->loc = current_.loc;
    
    expect(TokenType::TYPEDEF, "Expected typedef");
    decl->aliased_type = std::make_unique<Type>(parse_type());
    decl->name = expect(TokenType::IDENTIFIER, "Expected type alias name").value;
    expect(TokenType::SEMICOLON, "Expected ;");
    
    return decl;
}

Type Parser::parse_type() {
    Type t = parse_base_type();
    
    // Pointer
    while (match(TokenType::STAR)) {
        Type ptr;
        ptr.kind = Type::POINTER;
        ptr.pointee = std::make_unique<Type>(std::move(t));
        t = std::move(ptr);
    }
    
    // Array
    if (match(TokenType::LBRACKET)) {
        Type arr;
        arr.kind = Type::ARRAY;
        arr.element = std::make_unique<Type>(std::move(t));
        if (check(TokenType::INTEGER)) {
            arr.array_size = std::stoi(current_.value);
            advance();
        } else {
            arr.array_size = -1; // Slice
        }
        expect(TokenType::RBRACKET, "Expected ]");
        t = std::move(arr);
    }
    
    return t;
}

Type Parser::parse_base_type() {
    Type t;
    
    switch (current_.type) {
        case TokenType::VOID: t.kind = Type::VOID; advance(); break;
        case TokenType::BOOL: t.kind = Type::BOOL; advance(); break;
        case TokenType::I8: t.kind = Type::I8; advance(); break;
        case TokenType::I16: t.kind = Type::I16; advance(); break;
        case TokenType::I32: t.kind = Type::I32; advance(); break;
        case TokenType::I64: t.kind = Type::I64; advance(); break;
        case TokenType::U8: t.kind = Type::U8; advance(); break;
        case TokenType::U16: t.kind = Type::U16; advance(); break;
        case TokenType::U32: t.kind = Type::U32; advance(); break;
        case TokenType::U64: t.kind = Type::U64; advance(); break;
        case TokenType::F32: t.kind = Type::F32; advance(); break;
        case TokenType::F64: t.kind = Type::F64; advance(); break;
        case TokenType::PTR: t.kind = Type::PTR; advance(); break;
        case TokenType::CONST: {
            advance();
            Type inner = parse_type();
            inner.is_const = true;
            return inner;
        }
        case TokenType::IDENTIFIER:
            t.kind = Type::NAMED;
            t.name = current_.value;
            advance();
            break;
        case TokenType::FN: {
            advance();
            t.kind = Type::FUNCTION;
            expect(TokenType::LPAREN, "Expected (");
            while (!check(TokenType::RPAREN)) {
                t.params.push_back(parse_type());
                if (!check(TokenType::RPAREN)) expect(TokenType::COMMA, "Expected ,");
            }
            expect(TokenType::RPAREN, "Expected )");
            expect(TokenType::ARROW, "Expected ->");
            t.return_type = std::make_unique<Type>(parse_type());
            break;
        }
        default:
            error("Expected type, got: " + current_.value);
    }
    
    return t;
}

StmtPtr Parser::parse_statement() {
    if (check(TokenType::LBRACE)) return parse_block();
    if (check(TokenType::IF)) return parse_if();
    if (check(TokenType::WHILE)) return parse_while();
    if (check(TokenType::DO)) return parse_do_while();
    if (check(TokenType::FOR)) return parse_for();
    if (check(TokenType::SWITCH)) return parse_switch();
    if (check(TokenType::RETURN)) return parse_return();
    if (check(TokenType::ASM)) {
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::ASM;
        stmt->loc = current_.loc;
        advance();
        expect(TokenType::LPAREN, "Expected (");
        
        // Parse assembly content (simple pass-through)
        std::string asm_code;
        int paren_depth = 1;
        
        while (paren_depth > 0 && !check(TokenType::END_OF_FILE)) {
            if (check(TokenType::RPAREN)) {
                paren_depth--;
                if (paren_depth == 0) break;
            } else if (check(TokenType::LPAREN)) {
                paren_depth++;
            }
            
            if (check(TokenType::STRING)) {
                asm_code += "\"" + current_.value + "\"";
            } else if (check(TokenType::CHAR)) {
                asm_code += "'" + current_.value + "'";
            } else {
                asm_code += current_.value;
            }
            asm_code += " "; // precise spacing not guaranteed but usually fine for C
            advance();
        }
        
        stmt->asm_string = asm_code;
        expect(TokenType::RPAREN, "Expected )");
        expect(TokenType::SEMICOLON, "Expected ;");
        return stmt;
    }
    if (check(TokenType::CASE)) {
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::CASE;
        stmt->loc = current_.loc;
        advance();
        stmt->case_value = parse_expression();
        expect(TokenType::COLON, "Expected :");
        return stmt;
    }
    if (check(TokenType::DEFAULT)) {
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::DEFAULT;
        stmt->loc = current_.loc;
        advance();
        expect(TokenType::COLON, "Expected :");
        return stmt;
    }
    if (check(TokenType::BREAK)) {
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::BREAK;
        stmt->loc = current_.loc;
        advance();
        expect(TokenType::SEMICOLON, "Expected ;");
        return stmt;
    }
    if (check(TokenType::CONTINUE)) {
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::CONTINUE;
        stmt->loc = current_.loc;
        advance();
        expect(TokenType::SEMICOLON, "Expected ;");
        return stmt;
    }
    if (check(TokenType::GOTO)) {
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::GOTO;
        stmt->loc = current_.loc;
        advance();
        stmt->label = expect(TokenType::IDENTIFIER, "Expected label").value;
        expect(TokenType::SEMICOLON, "Expected ;");
        return stmt;
    }
    
    // Check for variable declaration
    bool is_const = match(TokenType::CONST);
    bool is_static = match(TokenType::STATIC);
    
    if (is_const || is_static || 
        check(TokenType::I8) || check(TokenType::I16) || check(TokenType::I32) || check(TokenType::I64) ||
        check(TokenType::U8) || check(TokenType::U16) || check(TokenType::U32) || check(TokenType::U64) ||
        check(TokenType::F32) || check(TokenType::F64) || check(TokenType::BOOL) || check(TokenType::PTR)) {
        return parse_var_decl();
    }
    
    // Check for label or expression
    if (check(TokenType::IDENTIFIER)) {
        Token id = current_;
        advance();
        if (match(TokenType::COLON)) {
            auto stmt = std::make_unique<Stmt>();
            stmt->kind = Stmt::LABEL;
            stmt->label = id.value;
            stmt->loc = id.loc;
            return stmt;
        }
        
        // Check if this might be a variable declaration with a named type
        if (check(TokenType::IDENTIFIER)) {
            // Named type followed by identifier = variable declaration
            // Put back the type info and parse as var decl
            auto stmt = std::make_unique<Stmt>();
            stmt->kind = Stmt::VAR_DECL;
            stmt->loc = id.loc;
            
            Type t;
            t.kind = Type::NAMED;
            t.name = id.value;
            stmt->var_type = std::make_unique<Type>(std::move(t));
            stmt->var_name = current_.value;
            advance();
            
            if (match(TokenType::ASSIGN)) {
                stmt->var_init = parse_expression();
            }
            expect(TokenType::SEMICOLON, "Expected ;");
            return stmt;
        }
        
        // It's an expression starting with identifier
        auto stmt = std::make_unique<Stmt>();
        stmt->kind = Stmt::EXPR;
        stmt->loc = id.loc;
        
        // Build identifier expression
        auto left = std::make_unique<Expr>();
        left->kind = Expr::IDENTIFIER;
        left->string_value = id.value;
        left->loc = id.loc;
        
        // Continue parsing postfix operators
        while (true) {
            if (match(TokenType::LPAREN)) {
                auto expr = std::make_unique<Expr>();
                expr->kind = Expr::CALL;
                expr->left = std::move(left);
                while (!check(TokenType::RPAREN)) {
                    expr->args.push_back(parse_expression());
                    if (!check(TokenType::RPAREN)) expect(TokenType::COMMA, "Expected ,");
                }
                expect(TokenType::RPAREN, "Expected )");
                left = std::move(expr);
            } else if (match(TokenType::LBRACKET)) {
                auto expr = std::make_unique<Expr>();
                expr->kind = Expr::INDEX;
                expr->left = std::move(left);
                expr->right = parse_expression();
                expect(TokenType::RBRACKET, "Expected ]");
                left = std::move(expr);
            } else if (match(TokenType::DOT)) {
                auto expr = std::make_unique<Expr>();
                expr->kind = Expr::MEMBER;
                expr->left = std::move(left);
                expr->string_value = expect(TokenType::IDENTIFIER, "Expected member name").value;
                left = std::move(expr);
            } else if (match(TokenType::ARROW)) {
                auto expr = std::make_unique<Expr>();
                expr->kind = Expr::MEMBER;
                expr->op = "->";
                expr->left = std::move(left);
                expr->string_value = expect(TokenType::IDENTIFIER, "Expected member name").value;
                left = std::move(expr);
            } else if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
                std::string op = current_.value;
                advance();
                auto expr = std::make_unique<Expr>();
                expr->kind = Expr::UNARY;
                expr->op = op + "_post";
                expr->operand = std::move(left);
                left = std::move(expr);
            } else {
                break;
            }
        }
        
        // Handle assignment and other binary operators
        if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) || 
            check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) ||
            check(TokenType::SLASH_ASSIGN)) {
            std::string op = current_.value;
            advance();
            auto right = parse_expression();
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::BINARY;
            expr->op = op;
            expr->left = std::move(left);
            expr->right = std::move(right);
            stmt->expr = std::move(expr);
        } else {
            stmt->expr = std::move(left);
        }
        
        expect(TokenType::SEMICOLON, "Expected ;");
        return stmt;
    }
    
    // Expression statement
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::EXPR;
    stmt->loc = current_.loc;
    stmt->expr = parse_expression();
    expect(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

StmtPtr Parser::parse_block() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::BLOCK;
    stmt->loc = current_.loc;
    
    expect(TokenType::LBRACE, "Expected {");
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        stmt->statements.push_back(parse_statement());
    }
    expect(TokenType::RBRACE, "Expected }");
    
    return stmt;
}

StmtPtr Parser::parse_if() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::IF;
    stmt->loc = current_.loc;
    
    expect(TokenType::IF, "Expected if");
    expect(TokenType::LPAREN, "Expected (");
    stmt->condition = parse_expression();
    expect(TokenType::RPAREN, "Expected )");
    stmt->then_stmt = parse_statement();
    
    if (match(TokenType::ELSE)) {
        stmt->else_stmt = parse_statement();
    }
    
    return stmt;
}

StmtPtr Parser::parse_while() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::WHILE;
    stmt->loc = current_.loc;
    
    expect(TokenType::WHILE, "Expected while");
    expect(TokenType::LPAREN, "Expected (");
    stmt->condition = parse_expression();
    expect(TokenType::RPAREN, "Expected )");
    stmt->body = parse_statement();
    
    return stmt;
}

StmtPtr Parser::parse_for() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::FOR;
    stmt->loc = current_.loc;
    
    expect(TokenType::FOR, "Expected for");
    expect(TokenType::LPAREN, "Expected (");
    
    if (!check(TokenType::SEMICOLON)) {
        stmt->init = parse_statement();
    } else {
        advance();
    }
    
    if (!check(TokenType::SEMICOLON)) {
        stmt->condition = parse_expression();
    }
    expect(TokenType::SEMICOLON, "Expected ;");
    
    if (!check(TokenType::RPAREN)) {
        stmt->post = parse_expression();
    }
    expect(TokenType::RPAREN, "Expected )");
    
    stmt->body = parse_statement();
    
    return stmt;
}

StmtPtr Parser::parse_return() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::RETURN;
    stmt->loc = current_.loc;
    
    expect(TokenType::RETURN, "Expected return");
    if (!check(TokenType::SEMICOLON)) {
        stmt->expr = parse_expression();
    }
    expect(TokenType::SEMICOLON, "Expected ;");
    
    return stmt;
}

StmtPtr Parser::parse_switch() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::SWITCH;
    stmt->loc = current_.loc;
    
    expect(TokenType::SWITCH, "Expected switch");
    expect(TokenType::LPAREN, "Expected (");
    stmt->condition = parse_expression();
    expect(TokenType::RPAREN, "Expected )");
    
    stmt->body = parse_statement();
    
    return stmt;
}

StmtPtr Parser::parse_do_while() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::DO_WHILE;
    stmt->loc = current_.loc;
    
    expect(TokenType::DO, "Expected do");
    stmt->body = parse_statement();
    expect(TokenType::WHILE, "Expected while");
    expect(TokenType::LPAREN, "Expected (");
    stmt->condition = parse_expression();
    expect(TokenType::RPAREN, "Expected )");
    expect(TokenType::SEMICOLON, "Expected ;");
    
    return stmt;
}

StmtPtr Parser::parse_var_decl() {
    auto stmt = std::make_unique<Stmt>();
    stmt->kind = Stmt::VAR_DECL;
    stmt->loc = current_.loc;
    stmt->is_const = false;
    stmt->is_static = false;
    
    stmt->var_type = std::make_unique<Type>(parse_type());
    stmt->var_name = expect(TokenType::IDENTIFIER, "Expected variable name").value;
    
    if (match(TokenType::ASSIGN)) {
        stmt->var_init = parse_expression();
    }
    
    expect(TokenType::SEMICOLON, "Expected ;");
    return stmt;
}

ExprPtr Parser::parse_expression() {
    return parse_assignment();
}

ExprPtr Parser::parse_assignment() {
    auto left = parse_ternary();
    
    if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) || 
        check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) ||
        check(TokenType::SLASH_ASSIGN)) {
        std::string op = current_.value;
        advance();
        auto right = parse_assignment();
        
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = op;
        expr->left = std::move(left);
        expr->right = std::move(right);
        return expr;
    }
    
    return left;
}

ExprPtr Parser::parse_ternary() {
    auto cond = parse_or();
    
    if (match(TokenType::QUESTION)) {
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::TERNARY;
        expr->condition = std::move(cond);
        expr->then_expr = parse_expression();
        expect(TokenType::COLON, "Expected :");
        expr->else_expr = parse_ternary();
        return expr;
    }
    
    return cond;
}

ExprPtr Parser::parse_or() {
    auto left = parse_and();
    while (match(TokenType::OR)) {
        auto right = parse_and();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = "||";
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_and() {
    auto left = parse_bitwise_or();
    while (match(TokenType::AND)) {
        auto right = parse_bitwise_or();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = "&&";
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_bitwise_or() {
    auto left = parse_bitwise_xor();
    while (match(TokenType::PIPE)) {
        auto right = parse_bitwise_xor();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = "|";
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_bitwise_xor() {
    auto left = parse_bitwise_and();
    while (match(TokenType::CARET)) {
        auto right = parse_bitwise_and();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = "^";
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_bitwise_and() {
    auto left = parse_equality();
    while (check(TokenType::AMP) && lexer_.peek().type != TokenType::AMP) {
        advance();
        auto right = parse_equality();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = "&";
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_equality() {
    auto left = parse_comparison();
    while (check(TokenType::EQ) || check(TokenType::NE)) {
        std::string op = current_.value;
        advance();
        auto right = parse_comparison();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = op;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_comparison() {
    auto left = parse_shift();
    while (check(TokenType::LT) || check(TokenType::GT) || 
           check(TokenType::LE) || check(TokenType::GE)) {
        std::string op = current_.value;
        advance();
        auto right = parse_shift();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = op;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_shift() {
    auto left = parse_additive();
    while (check(TokenType::LSHIFT) || check(TokenType::RSHIFT)) {
        std::string op = current_.value;
        advance();
        auto right = parse_additive();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = op;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_additive() {
    auto left = parse_multiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = current_.value;
        advance();
        auto right = parse_multiplicative();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = op;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_multiplicative() {
    auto left = parse_unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = current_.value;
        advance();
        auto right = parse_unary();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::BINARY;
        expr->op = op;
        expr->left = std::move(left);
        expr->right = std::move(right);
        left = std::move(expr);
    }
    return left;
}

ExprPtr Parser::parse_unary() {
    if (check(TokenType::MINUS) || check(TokenType::NOT) || check(TokenType::TILDE) ||
        check(TokenType::AMP) || check(TokenType::STAR) ||
        check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
        std::string op = current_.value;
        advance();
        auto operand = parse_unary();
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::UNARY;
        expr->op = op;
        expr->operand = std::move(operand);
        return expr;
    }
    
    if (check(TokenType::CAST)) {
        advance();
        expect(TokenType::LPAREN, "Expected (");
        Type type = parse_type();
        expect(TokenType::RPAREN, "Expected )");
        auto operand = parse_unary();
        
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::CAST;
        expr->cast_type = std::make_unique<Type>(std::move(type));
        expr->operand = std::move(operand);
        return expr;
    }
    
    if (check(TokenType::SIZEOF)) {
        advance();
        expect(TokenType::LPAREN, "Expected (");
        auto expr = std::make_unique<Expr>();
        expr->kind = Expr::SIZEOF;
        expr->cast_type = std::make_unique<Type>(parse_type());
        expect(TokenType::RPAREN, "Expected )");
        return expr;
    }
    
    return parse_postfix();
}

ExprPtr Parser::parse_postfix() {
    auto left = parse_primary();
    
    while (true) {
        if (match(TokenType::LPAREN)) {
            // Function call
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::CALL;
            expr->left = std::move(left);
            while (!check(TokenType::RPAREN)) {
                expr->args.push_back(parse_expression());
                if (!check(TokenType::RPAREN)) expect(TokenType::COMMA, "Expected ,");
            }
            expect(TokenType::RPAREN, "Expected )");
            left = std::move(expr);
        } else if (match(TokenType::LBRACKET)) {
            // Array index
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::INDEX;
            expr->left = std::move(left);
            expr->right = parse_expression();
            expect(TokenType::RBRACKET, "Expected ]");
            left = std::move(expr);
        } else if (match(TokenType::DOT)) {
            // Member access
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::MEMBER;
            expr->left = std::move(left);
            expr->string_value = expect(TokenType::IDENTIFIER, "Expected member name").value;
            left = std::move(expr);
        } else if (match(TokenType::ARROW)) {
            // Pointer member access
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::MEMBER;
            expr->op = "->";
            expr->left = std::move(left);
            expr->string_value = expect(TokenType::IDENTIFIER, "Expected member name").value;
            left = std::move(expr);
        } else if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
            std::string op = current_.value;
            advance();
            auto expr = std::make_unique<Expr>();
            expr->kind = Expr::UNARY;
            expr->op = op + "_post";
            expr->operand = std::move(left);
            left = std::move(expr);
        } else {
            break;
        }
    }
    
    return left;
}

ExprPtr Parser::parse_primary() {
    auto expr = std::make_unique<Expr>();
    expr->loc = current_.loc;
    
    if (check(TokenType::INTEGER)) {
        expr->kind = Expr::INTEGER_LIT;
        std::string val = current_.value;
        // Remove underscores
        val.erase(std::remove(val.begin(), val.end(), '_'), val.end());
        if (val.find("0x") == 0 || val.find("0X") == 0) {
            expr->int_value = std::stoll(val, nullptr, 16);
        } else if (val.find("0b") == 0 || val.find("0B") == 0) {
            expr->int_value = std::stoll(val.substr(2), nullptr, 2);
        } else if (val.find("0o") == 0 || val.find("0O") == 0) {
            expr->int_value = std::stoll(val.substr(2), nullptr, 8);
        } else {
            expr->int_value = std::stoll(val);
        }
        advance();
        return expr;
    }
    
    if (check(TokenType::FLOAT)) {
        expr->kind = Expr::FLOAT_LIT;
        std::string val = current_.value;
        val.erase(std::remove(val.begin(), val.end(), '_'), val.end());
        if (val.back() == 'f') val.pop_back();
        expr->float_value = std::stod(val);
        advance();
        return expr;
    }
    
    if (check(TokenType::STRING)) {
        expr->kind = Expr::STRING_LIT;
        expr->string_value = current_.value;
        advance();
        return expr;
    }
    
    if (check(TokenType::CHAR)) {
        expr->kind = Expr::CHAR_LIT;
        expr->string_value = current_.value;
        advance();
        return expr;
    }
    
    if (check(TokenType::TRUE) || check(TokenType::FALSE)) {
        expr->kind = Expr::BOOL_LIT;
        expr->bool_value = check(TokenType::TRUE);
        advance();
        return expr;
    }
    
    if (check(TokenType::NULL_LIT)) {
        expr->kind = Expr::NULL_LIT;
        advance();
        return expr;
    }
    
    if (check(TokenType::IDENTIFIER)) {
        expr->kind = Expr::IDENTIFIER;
        expr->string_value = current_.value;
        advance();
        return expr;
    }
    
    if (match(TokenType::LPAREN)) {
        expr = parse_expression();
        expect(TokenType::RPAREN, "Expected )");
        return expr;
    }
    
    if (match(TokenType::LBRACE)) {
        expr->kind = Expr::INIT_LIST;
        while (!check(TokenType::RBRACE)) {
            expr->elements.push_back(parse_expression());
            if (!check(TokenType::RBRACE)) expect(TokenType::COMMA, "Expected ,");
        }
        expect(TokenType::RBRACE, "Expected }");
        return expr;
    }
    
    error("Unexpected token: " + current_.value);
    return nullptr;
}

} // namespace cu
