#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <memory>
#include <unordered_map>

namespace cu {

// Source location for error reporting
struct SourceLoc {
    std::string file;
    int line = 1;
    int column = 1;
};

// Token types
enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    CHAR,
    TRUE,
    FALSE,
    NULL_LIT,
    
    // Identifiers
    IDENTIFIER,
    
    // Type keywords
    VOID, BOOL,
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    PTR,
    
    // Control flow
    IF, ELSE,
    WHILE, FOR,
    SWITCH, CASE, DEFAULT, DO,
    BREAK, CONTINUE,
    RETURN, GOTO,
    
    // Declarations
    FN, STRUCT, UNION, ENUM,
    CONST, STATIC, EXTERN, INLINE,
    TYPEDEF,
    
    // Special
    SIZEOF, TYPEOF, CAST,
    ASM, EXPORT, IMPORT,
    
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    AMP, PIPE, CARET, TILDE,
    LT, GT, LE, GE, EQ, NE,
    AND, OR, NOT,
    LSHIFT, RSHIFT,
    ASSIGN,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN, PERCENT_ASSIGN,
    AMP_ASSIGN, PIPE_ASSIGN, CARET_ASSIGN,
    LSHIFT_ASSIGN, RSHIFT_ASSIGN,
    INCREMENT, DECREMENT,
    ARROW,
    DOT,
    ELLIPSIS,
    QUESTION, COLON,
    COMMA,
    SEMICOLON,
    
    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    
    // Attributes
    AT,
    
    // Preprocessor
    HASH,
    
    // End of file
    END_OF_FILE,
    
    // Error
    ERROR
};

struct Token {
    TokenType type;
    std::string value;
    SourceLoc loc;
};

// Forward declarations for AST
struct Expr;
struct Stmt;
struct Decl;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;

// Type representation
struct Type {
    enum Kind {
        VOID, BOOL,
        I8, I16, I32, I64,
        U8, U16, U32, U64,
        F32, F64,
        PTR,
        POINTER,
        ARRAY,
        STRUCT,
        UNION,
        ENUM,
        FUNCTION,
        NAMED
    };
    
    Kind kind;
    bool is_const = false;               // const qualifier
    std::string name;                    // For named types
    std::unique_ptr<Type> pointee;       // For pointers
    std::unique_ptr<Type> element;       // For arrays
    int array_size = -1;                 // -1 = slice
    std::vector<Type> params;            // For function types
    std::unique_ptr<Type> return_type;   // For function types
};

// Expressions
struct Expr {
    enum Kind {
        INTEGER_LIT,
        FLOAT_LIT,
        STRING_LIT,
        CHAR_LIT,
        BOOL_LIT,
        NULL_LIT,
        IDENTIFIER,
        BINARY,
        UNARY,
        CALL,
        INDEX,
        MEMBER,
        CAST,
        SIZEOF,
        TERNARY,
        INIT_LIST
    };
    
    Kind kind;
    SourceLoc loc;
    
    // Literal values
    int64_t int_value = 0;
    double float_value = 0.0;
    std::string string_value;
    bool bool_value = false;
    
    // Binary/Unary
    std::string op;
    ExprPtr left;
    ExprPtr right;
    ExprPtr operand;
    
    // Call
    std::vector<ExprPtr> args;
    
    // Cast
    std::unique_ptr<Type> cast_type;
    
    // Ternary
    ExprPtr condition;
    ExprPtr then_expr;
    ExprPtr else_expr;
    
    // Init list
    std::vector<ExprPtr> elements;
};

// Statements
struct Stmt {
    enum Kind {
        EXPR,
        BLOCK,
        IF,
        WHILE,
        FOR,
        RETURN,
        BREAK,
        CONTINUE,
        GOTO,
        LABEL,
        VAR_DECL,
        SWITCH,
        CASE,
        DEFAULT,
        DO_WHILE,
        ASM
    };
    
    Kind kind;
    SourceLoc loc;
    
    // Expression statement
    ExprPtr expr;
    
    // Block
    std::vector<StmtPtr> statements;
    
    // If/While
    ExprPtr condition;
    StmtPtr then_stmt;
    StmtPtr else_stmt;
    
    // For
    StmtPtr init;
    ExprPtr post;
    StmtPtr body;
    
    // Goto/Label
    std::string label;
    
    // Variable declaration
    std::string var_name;
    std::unique_ptr<Type> var_type;
    ExprPtr var_init;
    bool is_const = false;
    bool is_static = false;
    
    // Asm
    std::string asm_string;
    
    // Case
    ExprPtr case_value;
};

// Declarations
struct Decl {
    enum Kind {
        FUNCTION,
        STRUCT,
        UNION,
        ENUM,
        TYPEDEF,
        IMPORT
    };
    
    Kind kind;
    SourceLoc loc;
    std::string name;
    
    // Attributes
    bool is_export = false;
    bool is_nomangle = false;
    bool is_inline = false;
    bool is_packed = false;
    int align = 0;
    
    // Function
    std::unique_ptr<Type> return_type;
    std::vector<std::pair<std::string, Type>> params;
    std::vector<StmtPtr> body;
    bool is_extern = false;
    bool is_variadic = false;
    
    // Struct/Union
    std::vector<std::pair<std::string, Type>> fields;
    
    // Enum
    std::vector<std::pair<std::string, std::optional<int64_t>>> enum_values;
    
    // Typedef
    std::unique_ptr<Type> aliased_type;
    
    // Import
    bool is_import = false;
};

// Translation unit
struct TranslationUnit {
    std::string filename;
    std::vector<DeclPtr> declarations;
};

} // namespace cu
