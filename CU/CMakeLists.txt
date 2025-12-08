cmake_minimum_required(VERSION 3.16)
project(cu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/codegen.cpp
)

# Executable
add_executable(cu ${SOURCES})

target_include_directories(cu PRIVATE src)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cu PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS cu DESTINATION bin)
