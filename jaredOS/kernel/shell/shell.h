/**
 * jaredOS - Shell Header
 */

#ifndef SHELL_H
#define SHELL_H

#include "../types.h"

/* Maximum command line length */
#define SHELL_MAX_LINE 256

/* Maximum arguments */
#define SHELL_MAX_ARGS 16

/* Run the shell */
void shell_run(void);

/* Print shell prompt */
void shell_prompt(void);

#endif /* SHELL_H */
