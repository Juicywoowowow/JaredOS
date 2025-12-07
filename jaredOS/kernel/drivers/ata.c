/**
 * jaredOS - ATA/IDE Driver Implementation (PIO Mode)
 */

#include "ata.h"
#include "../types.h"

/* Drive info */
static bool drive_present = false;

/**
 * Wait for drive to be ready
 */
static void ata_wait_ready(void) {
    while (inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
}

/**
 * Wait for data request
 */
static bool ata_wait_drq(void) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_STATUS_ERR) return false;
        if (status & ATA_STATUS_DRQ) return true;
    }
    return false;
}

/**
 * Initialize ATA driver
 */
bool ata_init(void) {
    /* Soft reset */
    outb(ATA_PRIMARY_CONTROL, 0x04);
    io_wait();
    io_wait();
    outb(ATA_PRIMARY_CONTROL, 0x00);

    /* Select drive 0 (master) */
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xA0);
    io_wait();

    /* Check if drive exists */
    outb(ATA_PRIMARY_SECTOR_COUNT, 0);
    outb(ATA_PRIMARY_LBA_LOW, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HIGH, 0);
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        drive_present = false;
        return false;
    }

    /* Wait for identify to complete */
    ata_wait_ready();
    
    /* Check for ATAPI (not supported) */
    if (inb(ATA_PRIMARY_LBA_MID) != 0 || inb(ATA_PRIMARY_LBA_HIGH) != 0) {
        drive_present = false;
        return false;
    }

    /* Wait for DRQ or error */
    if (!ata_wait_drq()) {
        drive_present = false;
        return false;
    }

    /* Read identify data (discard for now) */
    for (int i = 0; i < 256; i++) {
        inw(ATA_PRIMARY_DATA);
    }

    drive_present = true;
    return true;
}

/**
 * Check if drive present
 */
bool ata_drive_present(void) {
    return drive_present;
}

/**
 * Read sectors from disk (LBA28)
 */
bool ata_read_sectors(uint32_t lba, uint8_t count, void *buffer) {
    if (!drive_present || count == 0) return false;

    ata_wait_ready();

    /* Select drive and set high LBA bits */
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count and LBA */
    outb(ATA_PRIMARY_SECTOR_COUNT, count);
    outb(ATA_PRIMARY_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HIGH, (lba >> 16) & 0xFF);
    
    /* Send read command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);

    uint16_t *buf = (uint16_t*)buffer;
    
    for (int s = 0; s < count; s++) {
        /* Wait for data */
        if (!ata_wait_drq()) return false;
        
        /* Read 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            buf[s * 256 + i] = inw(ATA_PRIMARY_DATA);
        }
    }

    return true;
}

/**
 * Write sectors to disk (LBA28)
 */
bool ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer) {
    if (!drive_present || count == 0) return false;

    ata_wait_ready();

    /* Select drive and set high LBA bits */
    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Set sector count and LBA */
    outb(ATA_PRIMARY_SECTOR_COUNT, count);
    outb(ATA_PRIMARY_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA_HIGH, (lba >> 16) & 0xFF);
    
    /* Send write command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);

    const uint16_t *buf = (const uint16_t*)buffer;
    
    for (int s = 0; s < count; s++) {
        /* Wait for ready */
        if (!ata_wait_drq()) return false;
        
        /* Write 256 words (512 bytes) */
        for (int i = 0; i < 256; i++) {
            outw(ATA_PRIMARY_DATA, buf[s * 256 + i]);
        }
        
        /* Flush cache */
        outb(ATA_PRIMARY_COMMAND, 0xE7);
        ata_wait_ready();
    }

    return true;
}
