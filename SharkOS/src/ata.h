/*
 * =============================================================================
 * SharkOS ATA/IDE Driver Header (ata.h)
 * =============================================================================
 * Basic driver for reading/writing sectors to an ATA hard drive.
 * Uses PIO (Programmed I/O) mode.
 * =============================================================================
 */

#ifndef ATA_H
#define ATA_H

#include "types.h"

/* ----------------------------------------------------------------------------
 * ATA Constants
 * ---------------------------------------------------------------------------- */
#define ATA_SECTOR_SIZE     512

/* ----------------------------------------------------------------------------
 * ATA Functions
 * ---------------------------------------------------------------------------- */

/* Read a sector from the primary master drive (LBA28 addressing) */
void ata_read_sector(uint32_t lba, uint8_t* buffer);

/* Write a sector to the primary master drive (LBA28 addressing) */
void ata_write_sector(uint32_t lba, uint8_t* buffer, size_t size);

#endif /* ATA_H */
