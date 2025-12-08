; =============================================================================
; SharkOS Kernel Entry Point (kernel_entry.asm)
; =============================================================================
; This is the first kernel code that runs. It's called by the stage 2
; bootloader after switching to protected mode.
;
; What it does:
;   1. Calls kernel_main() (our C kernel)
;   2. Halts if kernel ever returns (it shouldn't)
;
; DEBUGGING TIPS:
;   - If kernel doesn't start, check if it's linked at correct address (0x1000)
;   - Verify the symbol 'kernel_main' is exported from kernel.c
;   - Use objdump -t to check symbol table
; =============================================================================

[bits 32]
[extern kernel_main]        ; Defined in kernel.c

section .text
global _start

_start:
    ; -------------------------------------------------------------------------
    ; Call our C kernel
    ; At this point we have:
    ;   - Protected mode enabled
    ;   - Flat memory model (all segments = 0-4GB)
    ;   - Stack set up at 0x90000
    ; -------------------------------------------------------------------------
    call kernel_main
    
    ; -------------------------------------------------------------------------
    ; Kernel returned? That's bad. Halt forever.
    ; This should never happen in a properly designed kernel
    ; -------------------------------------------------------------------------
.halt:
    cli                     ; Disable interrupts
    hlt                     ; Halt CPU
    jmp .halt               ; Loop just in case
