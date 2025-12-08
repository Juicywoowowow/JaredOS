/*
 * =============================================================================
 * SharkOS ATA/IDE Driver (ata.c)
 * =============================================================================
 * Implementation of ATA PIO mode driver.
 *
 * DEBUGGING TIPS:
 *   - LBA = Logical Block Address (sector number)
 *   - Primary IDE bus ports: 0x1F0 - 0x1F7
 *   - 0x1F7 is Status (read) and Command (write)
 *   - If stuck in wait loop, check if drive is attached in QEMU (-hda disk.img)
 * =============================================================================
 */

#include "ata.h"
#include "io.h"
#include "vga.h"

/* ----------------------------------------------------------------------------
 * ATA I/O Ports (Primary Bus)
 * ---------------------------------------------------------------------------- */
#define ATA_DATA        0x1F0
#define ATA_FEATURES    0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

/* ----------------------------------------------------------------------------
 * ATA Commands
 * ---------------------------------------------------------------------------- */
#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_FLUSH   0xE7

/* ----------------------------------------------------------------------------
 * ATA Status Bits
 * ---------------------------------------------------------------------------- */
#define ATA_SR_BSY      0x80    /* Busy */
#define ATA_SR_DRDY     0x40    /* Drive Ready */
#define ATA_SR_DRQ      0x08    /* Data Request ready */
#define ATA_SR_ERR      0x01    /* Error */

/* ----------------------------------------------------------------------------
 * ata_wait_busy - Wait until drive is not busy
 * ---------------------------------------------------------------------------- */
static void ata_wait_busy(void) {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

/* ----------------------------------------------------------------------------
 * ata_wait_drq - Wait until drive is ready to transfer data
 * ---------------------------------------------------------------------------- */
static void ata_wait_drq(void) {
    while (!(inb(ATA_STATUS) & ATA_SR_DRQ));
}

/* ----------------------------------------------------------------------------
 * ata_read_sector - Read one 512-byte sector
 * ---------------------------------------------------------------------------- */
void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_busy();

    /* Send LBA28 parameters */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); /* Slave bit=0, LBA mode=1 */
    outb(ATA_SECTOR_CNT, 1);                           /* Read 1 sector */
    outb(ATA_LBA_LO, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    
    /* Send read command */
    outb(ATA_COMMAND, ATA_CMD_READ);
    
    ata_wait_busy();
    ata_wait_drq();
    
    /* Read data (256 words = 512 bytes) */
    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        *ptr++ = inw(ATA_DATA);
    }
}

/* ----------------------------------------------------------------------------
 * ata_write_sector - Write one 512-byte sector
 * 
 * Note: size parameter is currently ignored as we always write 512 bytes,
 * but kept for future partial updates if needed.
 * ---------------------------------------------------------------------------- */
void ata_write_sector(uint32_t lba, uint8_t* buffer, size_t size) {
    ata_wait_busy();
    
    /* Send LBA28 parameters */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    
    /* Send write command */
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    
    ata_wait_busy();
    ata_wait_drq();
    
    /* Write data (256 words = 512 bytes) */
    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(ATA_DATA, *ptr++);
    }
    
    /* Send flush command */
    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    ata_wait_busy();
}
