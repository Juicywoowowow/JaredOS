#pragma once

#include "cu.hpp"
#include <string>
#include <sstream>

namespace cu {

class CodeGen {
public:
    std::string generate(const TranslationUnit& unit);
    
private:
    std::stringstream out_;
    int indent_ = 0;
    std::string source_file_;  // For trap error messages
    
    void emit(const std::string& s);
    void emit_line(const std::string& s);
    void emit_indent();
    void inc_indent();
    void dec_indent();
    
    void emit_decl(const Decl& decl);
    void emit_function(const Decl& decl);
    void emit_struct(const Decl& decl);
    void emit_union(const Decl& decl);
    void emit_enum(const Decl& decl);
    void emit_typedef(const Decl& decl);
    void emit_import(const Decl& decl);
    
    void emit_stmt(const Stmt& stmt);
    void emit_block(const Stmt& stmt);
    void emit_expr(const Expr& expr);
    
    // Emit trap check for null pointers
    void emit_null_check(const std::string& ptr_expr, int line);
    // Emit trap check for division
    void emit_div_check(const std::string& divisor_expr, int line);
    
    std::string type_to_c(const Type& type);
    std::string type_for_decl(const Type& type, const std::string& name);
};

} // namespace cu
