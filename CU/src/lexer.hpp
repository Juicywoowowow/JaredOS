#pragma once

#include "cu.hpp"
#include <string>
#include <vector>

namespace cu {

class Lexer {
public:
    Lexer(const std::string& source, const std::string& filename = "<input>");
    
    Token next();
    Token peek();
    bool is_eof() const;
    
    // For error reporting
    std::string get_line(int line_number) const;
    const std::string& source() const { return source_; }
    
private:
    std::string source_;
    std::string filename_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    std::optional<Token> peeked_;
    
    char current() const;
    char advance();
    bool match(char expected);
    void skip_whitespace();
    void skip_line_comment();
    void skip_block_comment();
    
    Token make_token(TokenType type, const std::string& value = "");
    Token scan_identifier();
    Token scan_number();
    Token scan_string();
    Token scan_char();
    
    TokenType keyword_or_identifier(const std::string& str);
};

} // namespace cu
