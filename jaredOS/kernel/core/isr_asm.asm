; =============================================================================
; jaredOS - ISR Assembly Stubs
; =============================================================================

[bits 32]
section .text

extern isr_handler

; Macro for ISR without error code
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0            ; Dummy error code
    push dword %1           ; Interrupt number
    jmp isr_common
%endmacro

; Macro for ISR with error code
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1           ; Interrupt number (error code already pushed)
    jmp isr_common
%endmacro

; Define ISRs 0-31
ISR_NOERR 0     ; Division By Zero
ISR_NOERR 1     ; Debug
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; Breakpoint
ISR_NOERR 4     ; Overflow
ISR_NOERR 5     ; Out of Bounds
ISR_NOERR 6     ; Invalid Opcode
ISR_NOERR 7     ; No Coprocessor
ISR_ERR   8     ; Double Fault
ISR_NOERR 9     ; Coprocessor Segment Overrun
ISR_ERR   10    ; Bad TSS
ISR_ERR   11    ; Segment Not Present
ISR_ERR   12    ; Stack Fault
ISR_ERR   13    ; General Protection Fault
ISR_ERR   14    ; Page Fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; Coprocessor Fault
ISR_NOERR 17    ; Alignment Check
ISR_NOERR 18    ; Machine Check
ISR_NOERR 19    ; Reserved
ISR_NOERR 20    ; Reserved
ISR_NOERR 21    ; Reserved
ISR_NOERR 22    ; Reserved
ISR_NOERR 23    ; Reserved
ISR_NOERR 24    ; Reserved
ISR_NOERR 25    ; Reserved
ISR_NOERR 26    ; Reserved
ISR_NOERR 27    ; Reserved
ISR_NOERR 28    ; Reserved
ISR_NOERR 29    ; Reserved
ISR_NOERR 30    ; Reserved
ISR_NOERR 31    ; Reserved

; Common ISR handler
isr_common:
    pusha                   ; Push all general-purpose registers

    mov ax, ds
    push eax                ; Save data segment

    mov ax, 0x10            ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                ; Push pointer to registers (parameter)
    call isr_handler
    add esp, 4              ; Clean up parameter

    pop eax                 ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                    ; Restore registers
    add esp, 8              ; Clean up error code and ISR number
    sti
    iret
