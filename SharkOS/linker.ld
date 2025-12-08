/*
 * =============================================================================
 * SharkOS Linker Script (linker.ld)
 * =============================================================================
 * Defines how the kernel binary is laid out in memory.
 *
 * Key addresses:
 *   0x1000 = Kernel load address (where bootloader loads us)
 *   0xB8000 = VGA text buffer (hardware-defined)
 *
 * DEBUGGING TIPS:
 *   - Use objdump -h shark.bin to see section layout
 *   - Use objdump -t shark.bin to see symbol addresses
 *   - If kernel doesn't run, verify .text starts at 0x1000
 *   - Check that ENTRY(_start) matches your kernel_entry.asm symbol
 * =============================================================================
 */

/* Entry point - the first code to run */
ENTRY(_start)

/* Sections layout */
SECTIONS {
    /* Start at 0x1000 - this is where the bootloader loads us */
    . = 0x1000;
    
    /* Code section - must come first as bootloader jumps here */
    .text : {
        *(.text)
    }
    
    /* Read-only data (string literals, const variables) */
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data */
    .data : {
        *(.data)
    }
    
    /* Uninitialized data (zeroed at startup) */
    .bss : {
        *(.bss)
        *(COMMON)
    }
}
