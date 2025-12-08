; =============================================================================
; kernel_entry.asm - Kernel Entry Point for FoxOS
; =============================================================================
;
; This assembly file serves as the bridge between the bootloader and the
; C kernel. It's linked at the beginning of the kernel binary and provides:
;   1. The entry point that bootloader jumps to
;   2. Stack setup
;   3. Call to kmain() in C
;   4. GDT/IDT loading helpers
;   5. Interrupt handler stubs
;
; DEBUGGING TIPS:
;   - If the kernel hangs immediately, check that this is linked first
;   - Verify the linker script puts _start at the beginning
;   - Use QEMU's -d int to see if interrupts are firing correctly
;
; =============================================================================

[BITS 32]                       ; We're already in 32-bit Protected Mode

; =============================================================================
; SECTION 1: Kernel Entry Point
; =============================================================================

section .text
global _start                   ; Entry point for linker
extern kmain                    ; C main function

_start:
    ; Set up kernel stack
    mov esp, kernel_stack_top
    
    ; Clear EFLAGS
    push 0
    popfd
    
    ; Call our C kernel main function
    call kmain
    
    ; If kmain returns, halt the system
    cli
.hang:
    hlt
    jmp .hang

; =============================================================================
; SECTION 2: GDT Helper Functions
; =============================================================================

global gdt_flush
extern gdt_ptr                  ; Defined in kernelA.c

; gdt_flush - Load new GDT and update segment registers
; Called from C: gdt_flush()
gdt_flush:
    lgdt [gdt_ptr]
    
    ; Reload segment registers
    mov ax, 0x10                ; Data segment selector (offset 16 in GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS
    jmp 0x08:.flush             ; Code segment selector (offset 8 in GDT)
.flush:
    ret

; =============================================================================
; SECTION 3: IDT Helper Functions
; =============================================================================

global idt_flush
extern idt_ptr                  ; Defined in kernelA.c

; idt_flush - Load new IDT
; Called from C: idt_flush()
idt_flush:
    lidt [idt_ptr]
    ret

; =============================================================================
; SECTION 4: Interrupt Service Routine Stubs
; =============================================================================
; These are the actual interrupt handlers that get called by the CPU.
; They save state, call the C handler, then restore state.
;
; Some interrupts push an error code, some don't. We push a dummy 0
; for those that don't to keep the stack layout consistent.

; Macro for ISRs that don't push an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0                ; Dummy error code
    push dword %1               ; Interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs that push an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1               ; Interrupt number (error code already pushed)
    jmp isr_common_stub
%endmacro

; Exception ISRs (0-31)
ISR_NOERRCODE 0                 ; Division by zero
ISR_NOERRCODE 1                 ; Debug
ISR_NOERRCODE 2                 ; NMI
ISR_NOERRCODE 3                 ; Breakpoint
ISR_NOERRCODE 4                 ; Overflow
ISR_NOERRCODE 5                 ; Bound range exceeded
ISR_NOERRCODE 6                 ; Invalid opcode
ISR_NOERRCODE 7                 ; Device not available
ISR_ERRCODE   8                 ; Double fault
ISR_NOERRCODE 9                 ; Coprocessor segment overrun
ISR_ERRCODE   10                ; Invalid TSS
ISR_ERRCODE   11                ; Segment not present
ISR_ERRCODE   12                ; Stack fault
ISR_ERRCODE   13                ; General protection fault
ISR_ERRCODE   14                ; Page fault
ISR_NOERRCODE 15                ; Reserved
ISR_NOERRCODE 16                ; x87 FPU error
ISR_ERRCODE   17                ; Alignment check
ISR_NOERRCODE 18                ; Machine check
ISR_NOERRCODE 19                ; SIMD FPU exception
ISR_NOERRCODE 20                ; Virtualization exception
ISR_NOERRCODE 21                ; Reserved
ISR_NOERRCODE 22                ; Reserved
ISR_NOERRCODE 23                ; Reserved
ISR_NOERRCODE 24                ; Reserved
ISR_NOERRCODE 25                ; Reserved
ISR_NOERRCODE 26                ; Reserved
ISR_NOERRCODE 27                ; Reserved
ISR_NOERRCODE 28                ; Reserved
ISR_NOERRCODE 29                ; Reserved
ISR_ERRCODE   30                ; Security exception
ISR_NOERRCODE 31                ; Reserved

; =============================================================================
; SECTION 5: IRQ Handler Stubs (Hardware Interrupts)
; =============================================================================
; IRQs 0-15 are remapped to interrupts 32-47

%macro IRQ 2
global irq%1
irq%1:
    push dword 0                ; Dummy error code
    push dword %2               ; Interrupt number
    jmp irq_common_stub
%endmacro

IRQ 0, 32                       ; Timer
IRQ 1, 33                       ; Keyboard
IRQ 2, 34                       ; Cascade (used internally)
IRQ 3, 35                       ; COM2
IRQ 4, 36                       ; COM1
IRQ 5, 37                       ; LPT2
IRQ 6, 38                       ; Floppy
IRQ 7, 39                       ; LPT1
IRQ 8, 40                       ; RTC
IRQ 9, 41                       ; Available
IRQ 10, 42                      ; Available
IRQ 11, 43                      ; Available
IRQ 12, 44                      ; PS/2 Mouse
IRQ 13, 45                      ; FPU
IRQ 14, 46                      ; Primary IDE
IRQ 15, 47                      ; Secondary IDE

; =============================================================================
; SECTION 6: Common Interrupt Handler
; =============================================================================

extern isr_handler              ; C function to handle ISRs
extern irq_handler              ; C function to handle IRQs

; Common stub for all exceptions
isr_common_stub:
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push pointer to stack frame (registers_t*)
    push esp
    
    ; Call C handler
    call isr_handler
    
    ; Remove pushed pointer
    add esp, 4
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Clean up error code and ISR number
    add esp, 8
    
    ; Return from interrupt
    iret

; Common stub for all IRQs
irq_common_stub:
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push pointer to stack frame
    push esp
    
    ; Call C handler
    call irq_handler
    
    ; Remove pushed pointer
    add esp, 4
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Clean up error code and IRQ number
    add esp, 8
    
    ; Return from interrupt
    iret

; =============================================================================
; SECTION 7: Kernel Stack
; =============================================================================

section .bss
align 16
kernel_stack_bottom:
    resb 16384                  ; 16KB kernel stack
kernel_stack_top:
