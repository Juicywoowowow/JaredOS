/**
 * jaredOS - Simple Filesystem Header
 */

#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include "../types.h"

/* Filesystem constants */
#define FS_MAX_FILES      16
#define FS_MAX_FILENAME   31
#define FS_SECTOR_SIZE    512
#define FS_SUPERBLOCK_SEC 0
#define FS_FILETABLE_SEC  1
#define FS_DATA_START_SEC 17   /* After superblock + file table */
#define FS_MAGIC          0x4A415245  /* "JARE" */

/* File entry structure */
typedef struct {
    char name[FS_MAX_FILENAME + 1];
    uint32_t size;
    uint32_t start_sector;
    uint8_t  used;
    uint8_t  padding[22];  /* Pad to 64 bytes */
} __attribute__((packed)) fs_file_t;

/* Superblock structure */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t file_count;
    uint32_t next_data_sector;
    uint8_t  padding[496];  /* Pad to 512 bytes */
} __attribute__((packed)) fs_superblock_t;

/* Initialize filesystem */
bool fs_init(void);

/* Format disk with filesystem */
bool fs_format(void);

/* List files */
int fs_list(fs_file_t *files, int max_files);

/* Get file info */
bool fs_stat(const char *name, fs_file_t *file);

/* Read file contents */
int fs_read(const char *name, void *buffer, uint32_t max_size);

/* Write file */
bool fs_write(const char *name, const void *data, uint32_t size);

/* Delete file */
bool fs_delete(const char *name);

/* Check if filesystem is ready */
bool fs_ready(void);

#endif /* SIMPLEFS_H */
