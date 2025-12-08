/*
 * =============================================================================
 * SharkOS Shell Header (shell.h)
 * =============================================================================
 * Simple command-line shell interface.
 * =============================================================================
 */

#ifndef SHELL_H
#define SHELL_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * Shell Constants
 * ---------------------------------------------------------------------------- */
#define SHELL_PROMPT        "shark> "
#define SHELL_MAX_INPUT     256
#define SHELL_MAX_ARGS      16

/* ----------------------------------------------------------------------------
 * Shell Functions
 * ---------------------------------------------------------------------------- */

/* Initialize and run the shell (does not return) */
void shell_run(void);

/* Process a single command line */
void shell_process_command(const char* line);

/* ----------------------------------------------------------------------------
 * Built-in Commands (implemented in shell.c)
 * ---------------------------------------------------------------------------- */
void cmd_help(int argc, char* argv[]);
void cmd_clear(int argc, char* argv[]);
void cmd_echo(int argc, char* argv[]);
void cmd_version(int argc, char* argv[]);
void cmd_reboot(int argc, char* argv[]);
void cmd_shutdown(int argc, char* argv[]);
void cmd_calc(int argc, char* argv[]);
void cmd_colors(int argc, char* argv[]);

#endif /* SHELL_H */
