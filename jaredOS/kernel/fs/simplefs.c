/**
 * jaredOS - Simple Filesystem Implementation
 */

#include "simplefs.h"
#include "../drivers/ata.h"
#include "../lib/string.h"

/* Filesystem state */
static fs_superblock_t superblock;
static fs_file_t file_table[FS_MAX_FILES];
static bool fs_initialized = false;
static char current_dir[FS_MAX_PATH] = "";  /* Current working directory */

/* Sector buffer */
static uint8_t sector_buffer[FS_SECTOR_SIZE];

/**
 * Read superblock and file table
 */
static bool fs_load_metadata(void) {
    /* Read superblock */
    if (!ata_read_sectors(FS_SUPERBLOCK_SEC, 1, &superblock)) {
        return false;
    }
    
    /* Check magic */
    if (superblock.magic != FS_MAGIC) {
        return false;
    }
    
    /* Read file table (32 files * 96 bytes = 3072 bytes = 6 sectors) */
    if (!ata_read_sectors(FS_FILETABLE_SEC, 6, file_table)) {
        return false;
    }
    
    return true;
}

/**
 * Write superblock and file table
 */
static bool fs_save_metadata(void) {
    /* Write superblock */
    if (!ata_write_sectors(FS_SUPERBLOCK_SEC, 1, &superblock)) {
        return false;
    }
    
    /* Write file table (6 sectors) */
    if (!ata_write_sectors(FS_FILETABLE_SEC, 6, file_table)) {
        return false;
    }
    
    return true;
}

/**
 * Initialize filesystem
 */
bool fs_init(void) {
    if (!ata_drive_present()) {
        fs_initialized = false;
        return false;
    }
    
    /* Try to load existing filesystem */
    if (fs_load_metadata()) {
        fs_initialized = true;
        return true;
    }
    
    /* No valid filesystem, need to format */
    fs_initialized = false;
    return false;
}

/**
 * Format disk with filesystem
 */
bool fs_format(void) {
    if (!ata_drive_present()) {
        return false;
    }
    
    /* Initialize superblock */
    memset(&superblock, 0, sizeof(superblock));
    superblock.magic = FS_MAGIC;
    superblock.version = 2;  /* v2 with directories */
    superblock.file_count = 0;
    superblock.next_data_sector = FS_DATA_START_SEC;
    
    /* Clear file table */
    memset(file_table, 0, sizeof(file_table));
    
    /* Reset current directory */
    current_dir[0] = '\0';
    
    /* Write metadata */
    if (!fs_save_metadata()) {
        return false;
    }
    
    fs_initialized = true;
    
    /* Create system directories */
    fs_mkdir("sys");
    fs_mkdir("bin");
    fs_mkdir("home");
    
    /* Create boot script */
    static const char boot_gw[] = 
        "; jaredOS Boot Script\n"
        "; This runs at startup\n"
        "@vga.print \"Boot script executed!\"\n"
        "@vga.newline\n";
    fs_write("sys/boot.gw", boot_gw, sizeof(boot_gw) - 1);
    
    /* Create example Gwango files in /bin */
    static const char hello_gw[] = 
        "; Hello World - Your first Gwango program!\n"
        "@vga.print \"Hello from Gwango!\"\n"
        "@vga.newline\n";
    fs_write("bin/hello.gw", hello_gw, sizeof(hello_gw) - 1);
    
    static const char math_gw[] = 
        "; Math Example - Variables and arithmetic\n"
        "var a = 10\n"
        "var b = 5\n"
        "var sum = a + b\n"
        "@vga.print \"Sum: \"\n"
        "@vga.print sum\n"
        "@vga.newline\n";
    fs_write("bin/math.gw", math_gw, sizeof(math_gw) - 1);
    
    static const char loop_gw[] = 
        "; Loop Example\n"
        "loop i = 1 to 5\n"
        "    @vga.print i\n"
        "    @vga.print \" \"\n"
        "end\n"
        "@vga.newline\n";
    fs_write("bin/loop.gw", loop_gw, sizeof(loop_gw) - 1);
    
    static const char input_gw[] = 
        "; Input Example - Press a key\n"
        "@vga.print \"Press any key: \"\n"
        "var k = @kb.getchar\n"
        "@vga.print \"You pressed ASCII: \"\n"
        "@vga.print k\n"
        "@vga.newline\n";
    fs_write("bin/input.gw", input_gw, sizeof(input_gw) - 1);
    
    return true;
}

/**
 * Check if filesystem is ready
 */
bool fs_ready(void) {
    return fs_initialized;
}

/**
 * List files
 */
int fs_list(fs_file_t *files, int max_files) {
    if (!fs_initialized) return 0;
    
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES && count < max_files; i++) {
        if (file_table[i].used) {
            if (files) {
                files[count] = file_table[i];
            }
            count++;
        }
    }
    return count;
}

/**
 * Get file info
 */
bool fs_stat(const char *name, fs_file_t *file) {
    if (!fs_initialized) return false;
    
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            if (file) *file = file_table[i];
            return true;
        }
    }
    return false;
}

/**
 * Read file contents
 */
int fs_read(const char *name, void *buffer, uint32_t max_size) {
    if (!fs_initialized) return -1;
    
    /* Find file */
    fs_file_t *file = NULL;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            file = &file_table[i];
            break;
        }
    }
    
    if (!file) return -1;
    
    /* Calculate how many bytes to read */
    uint32_t to_read = file->size;
    if (to_read > max_size) to_read = max_size;
    
    /* Read sectors */
    uint32_t sectors_needed = (to_read + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    uint8_t *buf = (uint8_t*)buffer;
    
    for (uint32_t s = 0; s < sectors_needed; s++) {
        if (!ata_read_sectors(file->start_sector + s, 1, sector_buffer)) {
            return -1;
        }
        
        uint32_t copy_size = FS_SECTOR_SIZE;
        if (s == sectors_needed - 1) {
            copy_size = to_read - (s * FS_SECTOR_SIZE);
        }
        memcpy(&buf[s * FS_SECTOR_SIZE], sector_buffer, copy_size);
    }
    
    return (int)to_read;
}

/**
 * Write file
 */
bool fs_write(const char *name, const void *data, uint32_t size) {
    if (!fs_initialized) return false;
    if (strlen(name) > FS_MAX_FILENAME) return false;
    
    /* Find existing file or free slot */
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            slot = i;
            break;
        }
    }
    
    /* Find free slot if not existing */
    if (slot < 0) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (!file_table[i].used) {
                slot = i;
                break;
            }
        }
    }
    
    if (slot < 0) return false;  /* No free slots */
    
    /* Calculate sectors needed */
    uint32_t sectors_needed = (size + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
    if (sectors_needed == 0) sectors_needed = 1;
    
    /* Allocate new sectors if new file */
    uint32_t start_sector;
    if (!file_table[slot].used) {
        start_sector = superblock.next_data_sector;
        superblock.next_data_sector += sectors_needed;
        superblock.file_count++;
    } else {
        start_sector = file_table[slot].start_sector;
    }
    
    /* Write data */
    const uint8_t *buf = (const uint8_t*)data;
    for (uint32_t s = 0; s < sectors_needed; s++) {
        memset(sector_buffer, 0, FS_SECTOR_SIZE);
        
        uint32_t copy_size = FS_SECTOR_SIZE;
        if ((s + 1) * FS_SECTOR_SIZE > size) {
            copy_size = size - (s * FS_SECTOR_SIZE);
        }
        memcpy(sector_buffer, &buf[s * FS_SECTOR_SIZE], copy_size);
        
        if (!ata_write_sectors(start_sector + s, 1, sector_buffer)) {
            return false;
        }
    }
    
    /* Update file table */
    strcpy(file_table[slot].name, name);
    file_table[slot].size = size;
    file_table[slot].start_sector = start_sector;
    file_table[slot].used = 1;
    
    /* Save metadata */
    return fs_save_metadata();
}

/**
 * Delete file
 */
bool fs_delete(const char *name) {
    if (!fs_initialized) return false;
    
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            file_table[i].used = 0;
            superblock.file_count--;
            return fs_save_metadata();
        }
    }
    
    return false;
}

/**
 * Create directory
 */
bool fs_mkdir(const char *name) {
    if (!fs_initialized) return false;
    if (strlen(name) > FS_MAX_FILENAME) return false;
    
    /* Check if already exists */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            return false;  /* Already exists */
        }
    }
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!file_table[i].used) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) return false;
    
    /* Create directory entry */
    strcpy(file_table[slot].name, name);
    file_table[slot].size = 0;
    file_table[slot].start_sector = 0;
    file_table[slot].used = 1;
    file_table[slot].is_dir = 1;
    superblock.file_count++;
    
    return fs_save_metadata();
}

/**
 * List files in a specific directory
 */
int fs_list_dir(const char *dir, fs_file_t *files, int max_files) {
    if (!fs_initialized) return 0;
    
    int dir_len = dir ? strlen(dir) : 0;
    int count = 0;
    
    for (int i = 0; i < FS_MAX_FILES && count < max_files; i++) {
        if (!file_table[i].used) continue;
        
        const char *name = file_table[i].name;
        
        if (dir_len == 0) {
            /* Root: show files without '/' or just the first component of paths */
            if (strchr(name, '/') == NULL) {
                if (files) files[count] = file_table[i];
                count++;
            }
        } else {
            /* Check if file is in this directory */
            if (strncmp(name, dir, dir_len) == 0 && name[dir_len] == '/') {
                /* Check if it's a direct child (no more slashes) */
                const char *rest = name + dir_len + 1;
                if (strchr(rest, '/') == NULL) {
                    if (files) files[count] = file_table[i];
                    count++;
                }
            }
        }
    }
    return count;
}

/**
 * Get current working directory
 */
const char* fs_getcwd(void) {
    return current_dir[0] ? current_dir : "/";
}

/**
 * Change current directory
 */
bool fs_chdir(const char *path) {
    if (!fs_initialized) return false;
    
    /* Handle root */
    if (path[0] == '/' && path[1] == '\0') {
        current_dir[0] = '\0';
        return true;
    }
    
    /* Handle ".." */
    if (strcmp(path, "..") == 0) {
        char *last_slash = strrchr(current_dir, '/');
        if (last_slash) {
            *last_slash = '\0';
        } else {
            current_dir[0] = '\0';
        }
        return true;
    }
    
    /* Build new path */
    char new_path[FS_MAX_PATH];
    if (path[0] == '/') {
        /* Absolute path */
        strncpy(new_path, path + 1, FS_MAX_PATH - 1);  /* Skip leading / */
    } else if (current_dir[0]) {
        /* Relative to current */
        strcpy(new_path, current_dir);
        strcat(new_path, "/");
        strcat(new_path, path);
    } else {
        strncpy(new_path, path, FS_MAX_PATH - 1);
    }
    new_path[FS_MAX_PATH - 1] = '\0';
    
    /* Check if directory exists */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && file_table[i].is_dir &&
            strcmp(file_table[i].name, new_path) == 0) {
            strcpy(current_dir, new_path);
            return true;
        }
    }
    
    return false;
}
