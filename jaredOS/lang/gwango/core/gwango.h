/**
 * Gwango Language - Main Runtime Header
 */

#ifndef GWANGO_H
#define GWANGO_H

#include "../../../kernel/types.h"

/* Run Gwango source code */
bool gwango_run(const char *source);

/* Run and dump generated x86 code */
bool gwango_dump(const char *source);

/* Dump generated x86 code from file */
bool gwango_dump_file(const char *filename);

/* Run Gwango file from filesystem */
bool gwango_run_file(const char *filename);

/* Enter REPL mode */
void gwango_repl(void);

#endif /* GWANGO_H */
