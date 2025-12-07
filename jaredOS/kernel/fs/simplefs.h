/**
 * jaredOS - Simple Filesystem Header
 */

#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include "../types.h"

/* Filesystem constants */
#define FS_MAX_FILES      32
#define FS_MAX_FILENAME   63   /* Longer paths like "sys/boot.gw" */
#define FS_MAX_PATH       64
#define FS_SECTOR_SIZE    512
#define FS_SUPERBLOCK_SEC 0
#define FS_FILETABLE_SEC  1
#define FS_DATA_START_SEC 33   /* After superblock + larger file table */
#define FS_MAGIC          0x4A415245  /* "JARE" */

/* File entry structure */
typedef struct {
    char name[FS_MAX_FILENAME + 1];
    uint32_t size;
    uint32_t start_sector;
    uint8_t  used;
    uint8_t  is_dir;        /* 1 if directory, 0 if file */
    uint8_t  padding[16];   /* Pad to 96 bytes */
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

/* List files in directory (NULL or "" for root) */
int fs_list(fs_file_t *files, int max_files);
int fs_list_dir(const char *dir, fs_file_t *files, int max_files);

/* Get file info */
bool fs_stat(const char *name, fs_file_t *file);

/* Read file contents */
int fs_read(const char *name, void *buffer, uint32_t max_size);

/* Write file */
bool fs_write(const char *name, const void *data, uint32_t size);

/* Create directory */
bool fs_mkdir(const char *name);

/* Delete file */
bool fs_delete(const char *name);

/* Check if filesystem is ready */
bool fs_ready(void);

/* Current directory */
const char* fs_getcwd(void);
bool fs_chdir(const char *path);

#endif /* SIMPLEFS_H */
