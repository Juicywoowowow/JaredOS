#pragma once

#include "cu.hpp"
#include "lexer.hpp"

namespace cu {

class Parser {
public:
    Parser(Lexer& lexer);
    
    TranslationUnit parse();
    
private:
    Lexer& lexer_;
    Token current_;
    
    void advance();
    bool check(TokenType type);
    bool match(TokenType type);
    Token expect(TokenType type, const std::string& msg);
    void error(const std::string& msg);
    
    // Declarations
    DeclPtr parse_declaration();
    DeclPtr parse_function();
    DeclPtr parse_struct();
    DeclPtr parse_union();
    DeclPtr parse_enum();
    DeclPtr parse_typedef();
    DeclPtr parse_import();
    std::vector<std::string> parse_attributes();
    
    // Types
    Type parse_type();
    Type parse_base_type();
    
    // Statements
    StmtPtr parse_statement();
    StmtPtr parse_block();
    StmtPtr parse_if();
    StmtPtr parse_while();
    StmtPtr parse_do_while();  // New
    StmtPtr parse_for();
    StmtPtr parse_switch();    // New
    StmtPtr parse_return();
    StmtPtr parse_var_decl();
    
    // Expressions
    ExprPtr parse_expression();
    ExprPtr parse_assignment();
    ExprPtr parse_ternary();
    ExprPtr parse_or();
    ExprPtr parse_and();
    ExprPtr parse_bitwise_or();
    ExprPtr parse_bitwise_xor();
    ExprPtr parse_bitwise_and();
    ExprPtr parse_equality();
    ExprPtr parse_comparison();
    ExprPtr parse_shift();
    ExprPtr parse_additive();
    ExprPtr parse_multiplicative();
    ExprPtr parse_unary();
    ExprPtr parse_postfix();
    ExprPtr parse_primary();
};

} // namespace cu
