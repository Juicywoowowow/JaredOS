/*
 * VBox - Simple x86 Emulator
 * Binary File Loader
 */

#include "vbox/vbox.h"
#include <stdio.h>
#include <stdlib.h>

/*============================================================================
 * Binary Loader
 *============================================================================*/

/**
 * Load a raw binary file into memory at the specified address
 */
VBoxError loader_load_binary(VBoxMemory *mem, const char *filename,
                             u32 load_addr) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
    return VBOX_ERR_FILE_NOT_FOUND;
  }

  /* Get file size */
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  /* Check size */
  if (file_size <= 0) {
    fprintf(stderr, "Error: File '%s' is empty\n", filename);
    fclose(fp);
    return VBOX_ERR_FILE_NOT_FOUND;
  }

  if (load_addr + file_size > VBOX_MEMORY_SIZE) {
    fprintf(stderr, "Error: File '%s' too large to load at 0x%05X\n", filename,
            load_addr);
    fclose(fp);
    return VBOX_ERR_FILE_TOO_LARGE;
  }

  /* Allocate temporary buffer */
  u8 *buffer = (u8 *)malloc(file_size);
  if (!buffer) {
    fprintf(stderr, "Error: Out of memory\n");
    fclose(fp);
    return VBOX_ERR_MEMORY;
  }

  /* Read file */
  size_t bytes_read = fread(buffer, 1, file_size, fp);
  fclose(fp);

  if (bytes_read != (size_t)file_size) {
    fprintf(stderr, "Error: Failed to read file '%s'\n", filename);
    free(buffer);
    return VBOX_ERR_FILE_NOT_FOUND;
  }

  /* Load into memory */
  mem_load(mem, load_addr, buffer, file_size);

  free(buffer);

  printf("Loaded %ld bytes from '%s' at 0x%05X\n", file_size, filename,
         load_addr);

  return VBOX_OK;
}

/**
 * Load a boot sector (expects 512 bytes, loads at 0x7C00)
 */
VBoxError loader_load_bootsector(VBoxMemory *mem, const char *filename) {
  return loader_load_binary(mem, filename, 0x7C00);
}
