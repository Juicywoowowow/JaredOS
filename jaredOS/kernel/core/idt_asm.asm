; =============================================================================
; jaredOS - IDT Assembly Routines
; =============================================================================

[bits 32]
section .text

global idt_load

; idt_load - Load IDT
; Parameter: pointer to IDT descriptor (on stack)
idt_load:
    mov eax, [esp+4]
    lidt [eax]
    ret
