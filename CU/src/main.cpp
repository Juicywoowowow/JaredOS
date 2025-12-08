#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

void print_usage() {
    std::cout << "CU Compiler v0.1.0\n";
    std::cout << "Usage: cu <file.cup> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>      Output C file to specified path\n";
    std::cout << "  --output <file> Same as -o\n";
    std::cout << "  --run          Transpile, compile and run (requires gcc/clang)\n";
    std::cout << "  --help         Show this help\n";
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << path << "\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        std::cerr << "Error: Cannot write to file: " << path << "\n";
        exit(1);
    }
    file << content;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    std::string input_file;
    std::string output_file;
    bool run_mode = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--run") {
            run_mode = true;
        } else if (arg[0] != '-') {
            input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }
    
    if (input_file.empty()) {
        std::cerr << "Error: No input file specified\n";
        return 1;
    }
    
    try {
        // Read source
        std::string source = read_file(input_file);
        
        // Lex
        cu::Lexer lexer(source, input_file);
        
        // Parse
        cu::Parser parser(lexer);
        cu::TranslationUnit unit = parser.parse();
        
        // Generate C code
        cu::CodeGen codegen;
        std::string c_code = codegen.generate(unit);
        
        if (run_mode) {
            // Write to temp file
            std::string temp_c = "/tmp/cu_temp.c";
            std::string temp_exe = "/tmp/cu_temp";
            write_file(temp_c, c_code);
            
            // Compile with gcc/clang
            std::string compile_cmd = "cc -o " + temp_exe + " " + temp_c + " 2>&1";
            int result = system(compile_cmd.c_str());
            if (result != 0) {
                std::cerr << "Compilation failed\n";
                return 1;
            }
            
            // Run
            return system(temp_exe.c_str());
        } else if (!output_file.empty()) {
            write_file(output_file, c_code);
            std::cout << "Generated: " << output_file << "\n";
        } else {
            std::cout << c_code;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
