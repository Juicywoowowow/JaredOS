/*
 * =============================================================================
 * SharkOS Simple File System (fs.c)
 * =============================================================================
 * Implementation of SimpleFS.
 * This is a minimal implementation for demonstration.
 * Limitations:
 *   - Max file size = 512 bytes (1 sector) for simplicity in this demo.
 *   - Only works in root directory.
 * =============================================================================
 */

#include "fs.h"
#include "ata.h"
#include "string.h"
#include "memory.h"
#include "vga.h"

/* ----------------------------------------------------------------------------
 * File System Superblock (cached in memory)
 * ---------------------------------------------------------------------------- */
static struct {
    uint16_t magic;
    uint32_t file_count;
    file_entry_t files[FS_MAX_FILES];
} sb;

/* ----------------------------------------------------------------------------
 * fs_flush_superblock - Write SB to Sector 0
 * ---------------------------------------------------------------------------- */
static void fs_flush_superblock(void) {
    /* We assume SB fits in one sector for this simple OS (it barely does) */
    /* sizeof(sb) = 2 + 4 + 32 * sizeof(file_entry_t) ~ 1.3KB -> Needs >1 sector! */
    /* FIX: Let's split across multiple sectors. */
    /* For now, just write enough sectors to cover it. */
    
    uint8_t* ptr = (uint8_t*)&sb;
    uint32_t sectors_needed = (sizeof(sb) + ATA_SECTOR_SIZE - 1) / ATA_SECTOR_SIZE;
    
    for (uint32_t i = 0; i < sectors_needed; i++) {
        ata_write_sector(i, ptr + (i * ATA_SECTOR_SIZE), ATA_SECTOR_SIZE);
    }
}

/* ----------------------------------------------------------------------------
 * fs_load_superblock - Read SB from Sector 0
 * ---------------------------------------------------------------------------- */
static void fs_load_superblock(void) {
    uint8_t* ptr = (uint8_t*)&sb;
    uint32_t sectors_needed = (sizeof(sb) + ATA_SECTOR_SIZE - 1) / ATA_SECTOR_SIZE;
    
    for (uint32_t i = 0; i < sectors_needed; i++) {
        ata_read_sector(i, ptr + (i * ATA_SECTOR_SIZE));
    }
}

/* ----------------------------------------------------------------------------
 * fs_init - Initialize FS
 * ---------------------------------------------------------------------------- */
void fs_init(void) {
    fs_load_superblock();
    
    if (sb.magic != FS_MAGIC) {
        vga_print("[FS] No valid filesystem found. Formatting...\n");
        
        memset(&sb, 0, sizeof(sb));
        sb.magic = FS_MAGIC;
        sb.file_count = 0;
        
        fs_flush_superblock();
        vga_print("[FS] Formatted successfully.\n");
    } else {
        vga_print("[FS] Filesystem mounted.\n");
    }
}

/* ----------------------------------------------------------------------------
 * fs_find_file - Helper to find file index
 * ---------------------------------------------------------------------------- */
static int fs_find_file(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (sb.files[i].used && strcmp(sb.files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* ----------------------------------------------------------------------------
 * fs_create - Create new file
 * ---------------------------------------------------------------------------- */
bool fs_create(const char* name) {
    if (fs_find_file(name) != -1) return false; /* Already exists */
    
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!sb.files[i].used) {
            strcpy(sb.files[i].name, name);
            sb.files[i].used = 1;
            sb.files[i].size = 0;
            sb.files[i].start_sector = FS_START_SECTOR + i; /* Each file gets 1 dedicated sector */
            sb.file_count++;
            
            fs_flush_superblock();
            return true;
        }
    }
    return false; /* FS full */
}

/* ----------------------------------------------------------------------------
 * fs_delete - Delete file
 * ---------------------------------------------------------------------------- */
bool fs_delete(const char* name) {
    int idx = fs_find_file(name);
    if (idx == -1) return false;
    
    sb.files[idx].used = 0;
    sb.file_count--;
    fs_flush_superblock();
    return true;
}

/* ----------------------------------------------------------------------------
 * fs_list - List files
 * ---------------------------------------------------------------------------- */
void fs_list(void) {
    vga_print("\n=== File List ===\n");
    bool found = false;
    
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (sb.files[i].used) {
            vga_print(sb.files[i].name);
            vga_print(" (");
            vga_print_int(sb.files[i].size);
            vga_print(" bytes)\n");
            found = true;
        }
    }
    
    if (!found) {
        vga_print("(empty)\n");
    }
    vga_print("\n");
}

/* ----------------------------------------------------------------------------
 * fs_write_file - Write data to file
 * ---------------------------------------------------------------------------- */
bool fs_write_file(const char* name, uint8_t* buffer, uint32_t size) {
    int idx = fs_find_file(name);
    if (idx == -1) return false;
    
    if (size > ATA_SECTOR_SIZE) {
        vga_print("Error: File too large (max 512 bytes)\n");
        return false;
    }
    
    /* Update size */
    sb.files[idx].size = size;
    fs_flush_superblock();
    
    /* Write content */
    ata_write_sector(sb.files[idx].start_sector, buffer, size);
    return true;
}

/* ----------------------------------------------------------------------------
 * fs_read_file - Read data from file
 * 
 * buffer must be at least ATA_SECTOR_SIZE bytes
 * ---------------------------------------------------------------------------- */
bool fs_read_file(const char* name, uint8_t* buffer) {
    int idx = fs_find_file(name);
    if (idx == -1) return false;
    
    ata_read_sector(sb.files[idx].start_sector, buffer);
    return true;
}

/* ----------------------------------------------------------------------------
 * fs_get_size - Get file size
 * ---------------------------------------------------------------------------- */
uint32_t fs_get_size(const char* name) {
    int idx = fs_find_file(name);
    if (idx == -1) return 0;
    return sb.files[idx].size;
}
