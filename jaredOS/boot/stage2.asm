; =============================================================================
; jaredOS Stage 2 Bootloader
; =============================================================================
; Enters protected mode and loads kernel from disk
; =============================================================================

[bits 16]
[org 0x7E00]

KERNEL_OFFSET equ 0x100000      ; Load kernel at 1MB
KERNEL_SECTORS equ 64           ; Sectors to load for kernel

stage2_start:
    ; Print stage2 message
    mov si, msg_stage2
    call print_string_16

    ; Enable A20 line
    call enable_a20

    ; Load kernel to temporary location (below 1MB first)
    mov si, msg_loading_kernel
    call print_string_16

    ; Load kernel to 0x10000 temporarily
    mov ax, 0x1000
    mov es, ax
    xor bx, bx                  ; ES:BX = 0x1000:0x0000 = 0x10000
    mov ah, 0x02
    mov al, KERNEL_SECTORS
    mov ch, 0                   ; Cylinder 0
    mov cl, 6                   ; Start from sector 6 (after stage1 + stage2)
    mov dh, 0                   ; Head 0
    mov dl, [0x7C00 + 72]       ; Get boot drive from stage1
    int 0x13
    jc disk_error_16

    ; Switch to protected mode
    cli                         ; Disable interrupts
    lgdt [gdt_descriptor]       ; Load GDT

    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code
    jmp 0x08:protected_mode

; =============================================================================
; Enable A20 Line
; =============================================================================
enable_a20:
    ; Try keyboard controller method
    call .wait_input
    mov al, 0xAD
    out 0x64, al

    call .wait_input
    mov al, 0xD0
    out 0x64, al

    call .wait_output
    in al, 0x60
    push ax

    call .wait_input
    mov al, 0xD1
    out 0x64, al

    call .wait_input
    pop ax
    or al, 2
    out 0x60, al

    call .wait_input
    mov al, 0xAE
    out 0x64, al

    call .wait_input
    ret

.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret

.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret

disk_error_16:
    mov si, msg_disk_error
    call print_string_16
    jmp $

; =============================================================================
; Print string in 16-bit mode
; =============================================================================
print_string_16:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

; =============================================================================
; 16-bit data
; =============================================================================
msg_stage2:         db "Stage 2...", 0x0D, 0x0A, 0
msg_loading_kernel: db "Loading kernel...", 0x0D, 0x0A, 0
msg_disk_error:     db "Disk read error!", 0x0D, 0x0A, 0

; =============================================================================
; GDT (Global Descriptor Table)
; =============================================================================
gdt_start:
    ; Null descriptor
    dq 0

gdt_code:
    ; Code segment descriptor
    dw 0xFFFF           ; Limit (low)
    dw 0x0000           ; Base (low)
    db 0x00             ; Base (middle)
    db 10011010b        ; Access byte
    db 11001111b        ; Flags + Limit (high)
    db 0x00             ; Base (high)

gdt_data:
    ; Data segment descriptor
    dw 0xFFFF           ; Limit (low)
    dw 0x0000           ; Base (low)
    db 0x00             ; Base (middle)
    db 10010010b        ; Access byte
    db 11001111b        ; Flags + Limit (high)
    db 0x00             ; Base (high)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size
    dd gdt_start                 ; GDT address

; =============================================================================
; 32-bit Protected Mode
; =============================================================================
[bits 32]

protected_mode:
    ; Set up segment registers
    mov ax, 0x10            ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; Stack pointer

    ; Copy kernel from 0x10000 to 0x100000 (1MB)
    mov esi, 0x10000
    mov edi, KERNEL_OFFSET
    mov ecx, (KERNEL_SECTORS * 512) / 4
    rep movsd

    ; Jump to kernel
    jmp KERNEL_OFFSET

; =============================================================================
; Padding
; =============================================================================
times 2048 - ($ - $$) db 0
