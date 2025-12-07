/**
 * jaredOS - Commands Header
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "../types.h"

/* Execute a command, returns true if command found */
bool commands_execute(int argc, char *argv[]);

/* Available commands */
void cmd_help(int argc, char *argv[]);
void cmd_clear(int argc, char *argv[]);
void cmd_echo(int argc, char *argv[]);
void cmd_about(int argc, char *argv[]);
void cmd_time(int argc, char *argv[]);
void cmd_reboot(int argc, char *argv[]);
void cmd_calc(int argc, char *argv[]);
void cmd_mem(int argc, char *argv[]);
void cmd_dump(int argc, char *argv[]);
void cmd_edit(int argc, char *argv[]);
void cmd_ls(int argc, char *argv[]);
void cmd_cat(int argc, char *argv[]);
void cmd_write(int argc, char *argv[]);
void cmd_format(int argc, char *argv[]);

#endif /* COMMANDS_H */
