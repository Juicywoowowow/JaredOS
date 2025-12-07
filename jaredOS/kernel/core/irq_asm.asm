; =============================================================================
; jaredOS - IRQ Assembly Stubs
; =============================================================================

[bits 32]
section .text

extern irq_handler

; Macro for IRQ stub
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0            ; Dummy error code
    push dword %2           ; Interrupt number (32+ for IRQs)
    jmp irq_common
%endmacro

; Define IRQ stubs (IRQ 0-15 map to INT 32-47)
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; Common IRQ handler
irq_common:
    pusha                   ; Push all registers

    mov ax, ds
    push eax                ; Save data segment

    mov ax, 0x10            ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                ; Push pointer to registers
    call irq_handler
    add esp, 4

    pop eax                 ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8              ; Clean up error code and IRQ number
    sti
    iret
