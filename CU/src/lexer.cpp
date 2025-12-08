#include "lexer.hpp"
#include <unordered_map>
#include <cctype>

namespace cu {

static const std::unordered_map<std::string, TokenType> keywords = {
    // Types
    {"void", TokenType::VOID}, {"bool", TokenType::BOOL},
    {"i8", TokenType::I8}, {"i16", TokenType::I16}, {"i32", TokenType::I32}, {"i64", TokenType::I64},
    {"u8", TokenType::U8}, {"u16", TokenType::U16}, {"u32", TokenType::U32}, {"u64", TokenType::U64},
    {"f32", TokenType::F32}, {"f64", TokenType::F64},
    {"ptr", TokenType::PTR},
    // Control flow
    {"if", TokenType::IF}, {"else", TokenType::ELSE},
    {"while", TokenType::WHILE}, {"for", TokenType::FOR},
    {"switch", TokenType::SWITCH}, {"case", TokenType::CASE},
    {"default", TokenType::DEFAULT}, {"do", TokenType::DO},
    {"break", TokenType::BREAK}, {"continue", TokenType::CONTINUE},
    {"return", TokenType::RETURN}, {"goto", TokenType::GOTO},
    // Declarations
    {"fn", TokenType::FN}, {"struct", TokenType::STRUCT},
    {"union", TokenType::UNION}, {"enum", TokenType::ENUM},
    {"const", TokenType::CONST}, {"static", TokenType::STATIC},
    {"extern", TokenType::EXTERN}, {"inline", TokenType::INLINE},
    {"typedef", TokenType::TYPEDEF},
    // Special
    {"sizeof", TokenType::SIZEOF}, {"typeof", TokenType::TYPEOF},
    {"cast", TokenType::CAST}, {"asm", TokenType::ASM},
    {"export", TokenType::EXPORT}, {"import", TokenType::IMPORT},
    // Literals
    {"true", TokenType::TRUE}, {"false", TokenType::FALSE},
    {"null", TokenType::NULL_LIT},
};

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source_(source), filename_(filename) {}

char Lexer::current() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::advance() {
    char c = current();
    pos_++;
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (current() == expected) {
        advance();
        return true;
    }
    return false;
}

void Lexer::skip_whitespace() {
    while (std::isspace(current())) advance();
}

void Lexer::skip_line_comment() {
    while (current() != '\n' && current() != '\0') advance();
}

void Lexer::skip_block_comment() {
    advance(); advance(); // Skip /*
    while (current() != '\0') {
        if (current() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            advance(); advance();
            return;
        }
        advance();
    }
}

Token Lexer::make_token(TokenType type, const std::string& value) {
    return Token{type, value, {filename_, line_, column_}};
}

TokenType Lexer::keyword_or_identifier(const std::string& str) {
    auto it = keywords.find(str);
    if (it != keywords.end()) return it->second;
    return TokenType::IDENTIFIER;
}

Token Lexer::scan_identifier() {
    size_t start = pos_;
    int start_col = column_;
    while (std::isalnum(current()) || current() == '_') advance();
    std::string value = source_.substr(start, pos_ - start);
    TokenType type = keyword_or_identifier(value);
    return Token{type, value, {filename_, line_, start_col}};
}

Token Lexer::scan_number() {
    size_t start = pos_;
    int start_col = column_;
    bool is_float = false;
    
    // Handle prefixes: 0x, 0b, 0o
    if (current() == '0' && pos_ + 1 < source_.size()) {
        char next = source_[pos_ + 1];
        if (next == 'x' || next == 'X') {
            advance(); advance();
            while (std::isxdigit(current()) || current() == '_') advance();
            return Token{TokenType::INTEGER, source_.substr(start, pos_ - start), {filename_, line_, start_col}};
        }
        if (next == 'b' || next == 'B') {
            advance(); advance();
            while (current() == '0' || current() == '1' || current() == '_') advance();
            return Token{TokenType::INTEGER, source_.substr(start, pos_ - start), {filename_, line_, start_col}};
        }
        if (next == 'o' || next == 'O') {
            advance(); advance();
            while ((current() >= '0' && current() <= '7') || current() == '_') advance();
            return Token{TokenType::INTEGER, source_.substr(start, pos_ - start), {filename_, line_, start_col}};
        }
    }
    
    while (std::isdigit(current()) || current() == '_') advance();
    
    // Decimal point
    if (current() == '.' && std::isdigit(source_[pos_ + 1])) {
        is_float = true;
        advance();
        while (std::isdigit(current()) || current() == '_') advance();
    }
    
    // Exponent
    if (current() == 'e' || current() == 'E') {
        is_float = true;
        advance();
        if (current() == '+' || current() == '-') advance();
        while (std::isdigit(current())) advance();
    }
    
    // Suffix
    if (current() == 'f') {
        is_float = true;
        advance();
    } else if (current() == 'u') {
        advance();
    }
    
    std::string value = source_.substr(start, pos_ - start);
    return Token{is_float ? TokenType::FLOAT : TokenType::INTEGER, value, {filename_, line_, start_col}};
}

Token Lexer::scan_string() {
    int start_col = column_;
    advance(); // Skip opening "
    std::string value;
    while (current() != '"' && current() != '\0') {
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '0': value += '\0'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case 'x': {
                    advance();
                    char hex[3] = {current(), '\0', '\0'};
                    advance();
                    hex[1] = current();
                    value += static_cast<char>(std::strtol(hex, nullptr, 16));
                    break;
                }
                default: value += current();
            }
        } else {
            value += current();
        }
        advance();
    }
    advance(); // Skip closing "
    return Token{TokenType::STRING, value, {filename_, line_, start_col}};
}

Token Lexer::scan_char() {
    int start_col = column_;
    advance(); // Skip opening '
    std::string value;
    if (current() == '\\') {
        advance();
        switch (current()) {
            case 'n': value += '\n'; break;
            case 't': value += '\t'; break;
            case 'r': value += '\r'; break;
            case '0': value += '\0'; break;
            case '\\': value += '\\'; break;
            case '\'': value += '\''; break;
            case 'x': {
                advance();
                char hex[3] = {current(), '\0', '\0'};
                advance();
                hex[1] = current();
                value += static_cast<char>(std::strtol(hex, nullptr, 16));
                break;
            }
            default: value += current();
        }
    } else {
        value += current();
    }
    advance();
    advance(); // Skip closing '
    return Token{TokenType::CHAR, value, {filename_, line_, start_col}};
}

Token Lexer::next() {
    if (peeked_) {
        Token t = *peeked_;
        peeked_.reset();
        return t;
    }
    
    skip_whitespace();
    
    // Comments
    while (current() == '/') {
        if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            skip_line_comment();
            skip_whitespace();
        } else if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            skip_block_comment();
            skip_whitespace();
        } else {
            break;
        }
    }
    
    if (pos_ >= source_.size()) {
        return make_token(TokenType::END_OF_FILE);
    }
    
    char c = current();
    
    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') return scan_identifier();
    
    // Numbers
    if (std::isdigit(c)) return scan_number();
    
    // Strings
    if (c == '"') return scan_string();
    
    // Characters
    if (c == '\'') return scan_char();
    
    // Operators and punctuation
    advance();
    switch (c) {
        case '+':
            if (match('+')) return make_token(TokenType::INCREMENT, "++");
            if (match('=')) return make_token(TokenType::PLUS_ASSIGN, "+=");
            return make_token(TokenType::PLUS, "+");
        case '-':
            if (match('-')) return make_token(TokenType::DECREMENT, "--");
            if (match('=')) return make_token(TokenType::MINUS_ASSIGN, "-=");
            if (match('>')) return make_token(TokenType::ARROW, "->");
            return make_token(TokenType::MINUS, "-");
        case '*':
            if (match('=')) return make_token(TokenType::STAR_ASSIGN, "*=");
            return make_token(TokenType::STAR, "*");
        case '/':
            if (match('=')) return make_token(TokenType::SLASH_ASSIGN, "/=");
            return make_token(TokenType::SLASH, "/");
        case '%':
            if (match('=')) return make_token(TokenType::PERCENT_ASSIGN, "%=");
            return make_token(TokenType::PERCENT, "%");
        case '&':
            if (match('&')) return make_token(TokenType::AND, "&&");
            if (match('=')) return make_token(TokenType::AMP_ASSIGN, "&=");
            return make_token(TokenType::AMP, "&");
        case '|':
            if (match('|')) return make_token(TokenType::OR, "||");
            if (match('=')) return make_token(TokenType::PIPE_ASSIGN, "|=");
            return make_token(TokenType::PIPE, "|");
        case '^':
            if (match('=')) return make_token(TokenType::CARET_ASSIGN, "^=");
            return make_token(TokenType::CARET, "^");
        case '~': return make_token(TokenType::TILDE, "~");
        case '<':
            if (match('<')) {
                if (match('=')) return make_token(TokenType::LSHIFT_ASSIGN, "<<=");
                return make_token(TokenType::LSHIFT, "<<");
            }
            if (match('=')) return make_token(TokenType::LE, "<=");
            return make_token(TokenType::LT, "<");
        case '>':
            if (match('>')) {
                if (match('=')) return make_token(TokenType::RSHIFT_ASSIGN, ">>=");
                return make_token(TokenType::RSHIFT, ">>");
            }
            if (match('=')) return make_token(TokenType::GE, ">=");
            return make_token(TokenType::GT, ">");
        case '=':
            if (match('=')) return make_token(TokenType::EQ, "==");
            return make_token(TokenType::ASSIGN, "=");
        case '!':
            if (match('=')) return make_token(TokenType::NE, "!=");
            return make_token(TokenType::NOT, "!");
        case '(': return make_token(TokenType::LPAREN, "(");
        case ')': return make_token(TokenType::RPAREN, ")");
        case '{': return make_token(TokenType::LBRACE, "{");
        case '}': return make_token(TokenType::RBRACE, "}");
        case '[': return make_token(TokenType::LBRACKET, "[");
        case ']': return make_token(TokenType::RBRACKET, "]");
        case ';': return make_token(TokenType::SEMICOLON, ";");
        case ':': return make_token(TokenType::COLON, ":");
        case ',': return make_token(TokenType::COMMA, ",");
        case '.':
            if (current() == '.' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '.') {
                advance(); advance();
                return make_token(TokenType::ELLIPSIS, "...");
            }
            return make_token(TokenType::DOT, ".");
        case '?': return make_token(TokenType::QUESTION, "?");
        case '@': return make_token(TokenType::AT, "@");
        case '#': return make_token(TokenType::HASH, "#");
    }
    
    return make_token(TokenType::ERROR, std::string(1, c));
}

Token Lexer::peek() {
    if (!peeked_) peeked_ = next();
    return *peeked_;
}

bool Lexer::is_eof() const {
    return pos_ >= source_.size();
}

std::string Lexer::get_line(int line_number) const {
    if (line_number < 1) return "";
    
    int current_line = 1;
    size_t line_start = 0;
    
    for (size_t i = 0; i < source_.size(); i++) {
        if (current_line == line_number) {
            line_start = i;
            break;
        }
        if (source_[i] == '\n') {
            current_line++;
        }
    }
    
    if (current_line != line_number) return "";
    
    // Find end of line
    size_t line_end = line_start;
    while (line_end < source_.size() && source_[line_end] != '\n') {
        line_end++;
    }
    
    return source_.substr(line_start, line_end - line_start);
}

} // namespace cu
