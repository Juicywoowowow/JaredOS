/**
 * jaredOS - Parser Header
 */

#ifndef PARSER_H
#define PARSER_H

#include "../types.h"

/* Parse command line into argc/argv */
void parse_command(char *line, int *argc, char *argv[], int max_args);

#endif /* PARSER_H */
