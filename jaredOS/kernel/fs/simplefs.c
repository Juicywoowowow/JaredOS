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
    
    /* Read file table (16 files * 64 bytes = 1024 bytes = 2 sectors) */
    if (!ata_read_sectors(FS_FILETABLE_SEC, 2, file_table)) {
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
    
    /* Write file table */
    if (!ata_write_sectors(FS_FILETABLE_SEC, 2, file_table)) {
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
    superblock.version = 1;
    superblock.file_count = 0;
    superblock.next_data_sector = FS_DATA_START_SEC;
    
    /* Clear file table */
    memset(file_table, 0, sizeof(file_table));
    
    /* Write metadata */
    if (!fs_save_metadata()) {
        return false;
    }
    
    fs_initialized = true;
    
    /* Create example Gwango files */
    static const char hello_gw[] = 
        "; Hello World - Your first Gwango program!\n"
        "@vga.print \"Hello from Gwango!\"\n"
        "@vga.newline\n";
    fs_write("hello.gw", hello_gw, sizeof(hello_gw) - 1);
    
    static const char math_gw[] = 
        "; Math Example - Variables and arithmetic\n"
        "var a = 10\n"
        "var b = 5\n"
        "var sum = a + b\n"
        "@vga.print \"Sum: \"\n"
        "@vga.print sum\n"
        "@vga.newline\n";
    fs_write("math.gw", math_gw, sizeof(math_gw) - 1);
    
    static const char loop_gw[] = 
        "; Loop Example\n"
        "loop i = 1 to 3\n"
        "    @vga.print i\n"
        "end\n"
        "@vga.newline\n";
    fs_write("loop.gw", loop_gw, sizeof(loop_gw) - 1);
    
    static const char input_gw[] = 
        "; Input Example - Press a key\n"
        "@vga.print \"Press any key: \"\n"
        "var k = @kb.getchar\n"
        "@vga.print \"You pressed ASCII: \"\n"
        "@vga.print k\n"
        "@vga.newline\n";
    fs_write("input.gw", input_gw, sizeof(input_gw) - 1);
    
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
