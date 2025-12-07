; =============================================================================
; jaredOS Stage 1 Bootloader
; =============================================================================
; 512-byte boot sector that loads stage2 from disk
; =============================================================================

[bits 16]
[org 0x7C00]

STAGE2_OFFSET equ 0x7E00        ; Where to load stage2
STAGE2_SECTORS equ 4            ; Number of sectors for stage2

start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; Stack grows down from boot sector

    ; Save boot drive
    mov [boot_drive], dl

    ; Print loading message
    mov si, msg_loading
    call print_string

    ; Load stage2 from disk
    mov ah, 0x02                ; BIOS read sectors
    mov al, STAGE2_SECTORS      ; Number of sectors to read
    mov ch, 0                   ; Cylinder 0
    mov cl, 2                   ; Start from sector 2 (sector 1 is boot)
    mov dh, 0                   ; Head 0
    mov dl, [boot_drive]        ; Drive number
    mov bx, STAGE2_OFFSET       ; ES:BX = destination
    int 0x13
    jc disk_error               ; Jump if carry flag set (error)

    ; Jump to stage2 (with boot drive in DL)
    mov dl, [boot_drive]
    jmp STAGE2_OFFSET

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp halt

halt:
    cli
    hlt
    jmp halt

; =============================================================================
; Print null-terminated string (SI = string pointer)
; =============================================================================
print_string:
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
; Data
; =============================================================================
boot_drive:     db 0
msg_loading:    db "jaredOS Loading...", 0x0D, 0x0A, 0
msg_disk_error: db "Disk Error!", 0x0D, 0x0A, 0

; =============================================================================
; Boot sector padding and signature
; =============================================================================
times 510 - ($ - $$) db 0
dw 0xAA55
