; =============================================================================
; jaredOS - GDT Assembly Routines
; =============================================================================

[bits 32]
section .text

global gdt_flush

; gdt_flush - Load GDT and reload segment registers
; Parameter: pointer to GDT descriptor (on stack)
gdt_flush:
    mov eax, [esp+4]        ; Get GDT pointer
    lgdt [eax]              ; Load GDT

    mov ax, 0x10            ; Data segment selector (offset 16)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush         ; Far jump to code segment (offset 8)
.flush:
    ret
