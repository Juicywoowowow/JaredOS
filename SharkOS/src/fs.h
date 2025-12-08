/*
 * =============================================================================
 * SharkOS Simple File System (fs.h)
 * =============================================================================
 * A flat, contiguous file system.
 * 
 * Structure on Disk (Sector 0):
 *   | Magic (2B) | Count (4B) | FileEntry[0] | FileEntry[1] ... |
 *
 * Each FileEntry is fixed size.
 * New files are allocated sequentially. No fragmentation handling (simple!).
 * =============================================================================
 */

#ifndef FS_H
#define FS_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * FS Constants
 * ---------------------------------------------------------------------------- */
#define FS_MAGIC        0x55AA
#define FS_MAX_FILES    32
#define FS_FILENAME_LEN 32
#define FS_START_SECTOR 100     /* Start of file data area */

/* ----------------------------------------------------------------------------
 * File Entry Structure
 * ---------------------------------------------------------------------------- */
typedef struct {
    char name[FS_FILENAME_LEN]; /* Filename (null-terminated) */
    uint32_t start_sector;      /* Starting LBA */
    uint32_t size;              /* Size in bytes */
    uint32_t used;              /* 1 if valid, 0 if free */
} file_entry_t;

/* ----------------------------------------------------------------------------
 * FS Functions
 * ---------------------------------------------------------------------------- */

/* Initialize filesystem (check magic, format if new) */
void fs_init(void);

/* List all files */
void fs_list(void);

/* Create an empty file */
bool fs_create(const char* name);

/* Delete a file */
bool fs_delete(const char* name);

/* Write data to a file */
bool fs_write_file(const char* name, uint8_t* buffer, uint32_t size);

/* Read data from a file */
bool fs_read_file(const char* name, uint8_t* buffer);

/* Get file size */
uint32_t fs_get_size(const char* name);

#endif /* FS_H */
