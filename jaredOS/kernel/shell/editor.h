/**
 * jaredOS - Text Editor Header
 */

#ifndef EDITOR_H
#define EDITOR_H

#include "../types.h"

/* Editor constants */
#define EDITOR_MAX_LINES 23
#define EDITOR_MAX_COLS  78
#define EDITOR_BUFFER_SIZE (EDITOR_MAX_LINES * (EDITOR_MAX_COLS + 1))

/* Open editor with optional filename */
void editor_open(const char *filename);

/* Get editor buffer (for saving) */
const char* editor_get_buffer(void);

#endif /* EDITOR_H */
