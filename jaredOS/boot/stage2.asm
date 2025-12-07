; =============================================================================
; jaredOS Stage 2 Bootloader
; =============================================================================
; Enters protected mode and loads kernel from disk
; =============================================================================

[bits 16]
[org 0x7E00]

KERNEL_OFFSET equ 0x100000      ; Load kernel at 1MB
KERNEL_SECTORS equ 128          ; Sectors to load for kernel (~64KB)

; Floppy geometry
SECTORS_PER_TRACK equ 18
HEADS equ 2

stage2_start:
    ; Save boot drive (passed from stage1 in DL)
    mov [boot_drive], dl

    ; Print stage2 message
    mov si, msg_stage2
    call print_string_16

    ; Enable A20 line
    call enable_a20

    ; Print loading message
    mov si, msg_loading_kernel
    call print_string_16

    ; Reset disk system
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; Load kernel to 0x10000 using multiple reads if needed
    mov bx, 0x1000              ; Segment
    mov es, bx
    xor bx, bx                  ; Offset = 0, so ES:BX = 0x10000

    mov cx, KERNEL_SECTORS      ; Total sectors to read
    mov byte [current_sector], 6  ; Starting sector (after stage1+stage2)
    mov byte [current_head], 0
    mov byte [current_cylinder], 0

.read_loop:
    cmp cx, 0
    je .read_done

    ; Calculate how many sectors we can read in this track
    mov al, [current_sector]
    mov ah, SECTORS_PER_TRACK + 1
    sub ah, al                  ; Remaining sectors in track
    
    ; Don't read more than we need
    cmp ah, cl
    jbe .use_ah
    mov ah, cl
.use_ah:
    mov [sectors_to_read], ah

    ; Read sectors
    push cx                     ; Save remaining count
    mov ah, 0x02                ; BIOS read sectors
    mov al, [sectors_to_read]
    mov ch, [current_cylinder]
    mov cl, [current_sector]
    mov dh, [current_head]
    mov dl, [boot_drive]
    int 0x13
    jc disk_error_16

    ; Update buffer pointer
    mov al, [sectors_to_read]
    xor ah, ah
    shl ax, 9                   ; Multiply by 512
    add bx, ax
    jnc .no_segment_overflow
    
    ; Handle segment overflow
    mov ax, es
    add ax, 0x1000
    mov es, ax
    xor bx, bx
.no_segment_overflow:

    ; Update sector position
    mov al, [current_sector]
    add al, [sectors_to_read]
    
    ; Check if we crossed a track
    cmp al, SECTORS_PER_TRACK + 1
    jb .same_track
    
    ; Move to next head/cylinder
    mov byte [current_sector], 1
    mov al, [current_head]
    inc al
    cmp al, HEADS
    jb .same_cylinder
    
    xor al, al                  ; Head 0
    inc byte [current_cylinder]
.same_cylinder:
    mov [current_head], al
    jmp .update_count
    
.same_track:
    mov [current_sector], al

.update_count:
    pop cx
    mov al, [sectors_to_read]
    xor ah, ah
    sub cx, ax
    jmp .read_loop

.read_done:
    ; Print success
    mov si, msg_ok
    call print_string_16

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
    ; Fast A20 method (port 0x92)
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

disk_error_16:
    mov si, msg_disk_error
    call print_string_16
    mov ah, 0x01                ; Get disk status
    mov dl, [boot_drive]
    int 0x13
    ; Print error code in AH
    mov al, ah
    call print_hex
    jmp $

; =============================================================================
; Print hex byte in AL
; =============================================================================
print_hex:
    push ax
    mov ah, 0x0E
    mov bx, 0
    
    push ax
    shr al, 4
    cmp al, 10
    jb .digit1
    add al, 7
.digit1:
    add al, '0'
    int 0x10
    pop ax
    
    and al, 0x0F
    cmp al, 10
    jb .digit2
    add al, 7
.digit2:
    add al, '0'
    int 0x10
    
    pop ax
    ret

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
boot_drive:         db 0
current_sector:     db 0
current_head:       db 0
current_cylinder:   db 0
sectors_to_read:    db 0

msg_stage2:         db "Stage 2...", 0x0D, 0x0A, 0
msg_loading_kernel: db "Loading kernel...", 0x0D, 0x0A, 0
msg_ok:             db "OK", 0x0D, 0x0A, 0
msg_disk_error:     db "Disk Error: ", 0

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
