/**
 * jaredOS - Parser Implementation
 */

#include "parser.h"
#include "../lib/stdlib.h"

/**
 * Parse command line into argc/argv
 */
void parse_command(char *line, int *argc, char *argv[], int max_args) {
    *argc = 0;
    
    while (*line && *argc < max_args) {
        /* Skip whitespace */
        while (*line && isspace(*line)) {
            line++;
        }
        
        if (*line == '\0') break;
        
        /* Start of argument */
        argv[*argc] = line;
        (*argc)++;
        
        /* Find end of argument */
        while (*line && !isspace(*line)) {
            line++;
        }
        
        /* Null-terminate argument */
        if (*line) {
            *line = '\0';
            line++;
        }
    }
}
