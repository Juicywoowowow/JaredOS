; =============================================================================
; SharkOS Stage 1 Bootloader (boot.asm)
; =============================================================================
; This is the first code that runs when the computer boots. It must fit in
; exactly 512 bytes (the Master Boot Record size).
;
; What it does:
;   1. Sets up segment registers
;   2. Loads stage 2 bootloader from disk
;   3. Jumps to stage 2
;
; DEBUGGING TIPS:
;   - If boot fails, check BIOS error codes in AH after INT 13h
;   - Use QEMU's -d int to debug interrupts
;   - If you see garbage on screen, check segment registers
;   - Boot signature 0xAA55 must be at bytes 510-511
; =============================================================================

[bits 16]                   ; We start in 16-bit real mode
[org 0x7C00]               ; BIOS loads us here

STAGE2_OFFSET equ 0x7E00   ; Where we'll load stage 2 (right after boot sector)
KERNEL_OFFSET equ 0x1000   ; Where kernel will be loaded (by stage 2)

start:
    ; -------------------------------------------------------------------------
    ; Initialize segment registers to 0
    ; IMPORTANT: Never assume register values at boot - BIOS leaves them random!
    ; -------------------------------------------------------------------------
    xor ax, ax              ; Zero out AX
    mov ds, ax              ; Data segment = 0
    mov es, ax              ; Extra segment = 0
    mov ss, ax              ; Stack segment = 0
    mov sp, 0x7C00          ; Stack grows down from where we're loaded
    
    ; -------------------------------------------------------------------------
    ; Save boot drive number (BIOS passes it in DL)
    ; We need this later to read more sectors
    ; -------------------------------------------------------------------------
    mov [BOOT_DRIVE], dl
    
    ; -------------------------------------------------------------------------
    ; Print boot message - let user know we're alive
    ; -------------------------------------------------------------------------
    mov si, MSG_BOOT
    call print_string
    
    ; -------------------------------------------------------------------------
    ; Load stage 2 bootloader from disk
    ; We read sectors 2-5 (4 sectors = 2KB) into memory at STAGE2_OFFSET
    ; -------------------------------------------------------------------------
    mov bx, STAGE2_OFFSET   ; ES:BX = destination address
    mov dh, 4               ; Read 4 sectors
    mov dl, [BOOT_DRIVE]    ; From boot drive
    mov cl, 2               ; Starting at sector 2 (sector 1 = boot sector)
    call disk_read
    
    ; -------------------------------------------------------------------------
    ; Jump to stage 2 bootloader
    ; Point of no return - stage 2 takes over from here
    ; -------------------------------------------------------------------------
    mov si, MSG_STAGE2
    call print_string
    jmp STAGE2_OFFSET

; =============================================================================
; print_string: Print null-terminated string to screen
; Input: SI = pointer to string
; Clobbers: AX, BX, SI
; =============================================================================
print_string:
    mov ah, 0x0E            ; BIOS teletype function
    mov bh, 0               ; Page number
.loop:
    lodsb                   ; Load byte from SI into AL, increment SI
    cmp al, 0               ; Check for null terminator
    je .done
    int 0x10                ; BIOS video interrupt
    jmp .loop
.done:
    ret

; =============================================================================
; disk_read: Read sectors from disk using BIOS INT 13h
; Input:
;   DL = drive number
;   DH = number of sectors to read
;   CL = starting sector (1-based)
;   ES:BX = destination buffer
; =============================================================================
disk_read:
    pusha                   ; Save all registers
    push dx                 ; Save DX (we need DH later for verification)
    
    mov ah, 0x02            ; BIOS read sectors function
    mov al, dh              ; Number of sectors to read
    mov ch, 0               ; Cylinder 0
    mov dh, 0               ; Head 0
    ; CL already contains sector number
    ; DL already contains drive number
    
    int 0x13                ; BIOS disk interrupt
    jc .disk_error          ; Jump if carry flag set (error)
    
    pop dx                  ; Restore DX
    cmp al, dh              ; Verify correct number of sectors read
    jne .sector_error
    
    popa                    ; Restore all registers
    ret

.disk_error:
    ; -------------------------------------------------------------------------
    ; DEBUGGING: Error codes in AH after failed INT 13h:
    ;   0x01 = Invalid command
    ;   0x02 = Cannot find address mark
    ;   0x03 = Write protected (shouldn't happen on read)
    ;   0x04 = Sector not found
    ;   0x05 = Reset failed
    ;   0x0C = Unsupported track
    ; -------------------------------------------------------------------------
    mov si, MSG_DISK_ERR
    call print_string
    jmp halt

.sector_error:
    mov si, MSG_SECT_ERR
    call print_string
    ; Fall through to halt

halt:
    cli                     ; Disable interrupts
    hlt                     ; Halt CPU
    jmp halt                ; In case we wake up, halt again

; =============================================================================
; Data Section
; =============================================================================
BOOT_DRIVE:     db 0
MSG_BOOT:       db '[SharkOS] Stage 1 loading...', 13, 10, 0
MSG_STAGE2:     db '[SharkOS] Jumping to Stage 2...', 13, 10, 0
MSG_DISK_ERR:   db '[ERROR] Disk read failed!', 13, 10, 0
MSG_SECT_ERR:   db '[ERROR] Wrong sector count!', 13, 10, 0

; =============================================================================
; Boot Sector Padding and Signature
; The boot sector MUST be exactly 512 bytes with 0xAA55 at the end
; =============================================================================
times 510-($-$$) db 0       ; Pad with zeros to byte 510
dw 0xAA55                   ; Boot signature (little-endian)
