/* jaredOS Linker Script */

ENTRY(kernel_main)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = 0x100000;   /* Kernel loads at 1MB */

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    kernel_end = .;
}
