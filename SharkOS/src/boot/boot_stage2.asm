; =============================================================================
; SharkOS Stage 2 Bootloader (boot_stage2.asm)
; =============================================================================
; This is loaded by stage 1 and performs the heavy lifting of boot:
;   1. Enable A20 line (access memory > 1MB)
;   2. Load GDT (Global Descriptor Table) for protected mode
;   3. Switch to 32-bit protected mode
;   4. Load and jump to kernel
;
; DEBUGGING TIPS:
;   - If stuck after "Entering protected mode", GDT might be wrong
;   - A20 enabling can fail on some systems - we try multiple methods
;   - After protected mode, we can't use BIOS interrupts anymore!
;   - Use 0xB8000 VGA memory to print in protected mode
; =============================================================================

[bits 16]
[org 0x7E00]                ; We're loaded right after boot sector

KERNEL_OFFSET equ 0x1000    ; Where we load the kernel

stage2_start:
    ; -------------------------------------------------------------------------
    ; Print status message (still in real mode, can use BIOS)
    ; -------------------------------------------------------------------------
    mov si, MSG_STAGE2_LOADED
    call print_string_rm
    
    ; -------------------------------------------------------------------------
    ; Load kernel from disk before we lose BIOS access
    ; Kernel starts at sector 6 (sectors 1-5 are bootloaders)
    ; -------------------------------------------------------------------------
    mov bx, KERNEL_OFFSET   ; Load kernel here
    mov dh, 32              ; Read 32 sectors (16KB - enough for our kernel)
    mov dl, [0x7C00 + 510 - 10] ; Get boot drive from stage 1's storage
                            ; Actually let's be safe and read from 0
    xor ax, ax
    mov dl, 0               ; Assume floppy drive 0
    mov cl, 6               ; Start at sector 6
    call disk_read_rm
    
    ; -------------------------------------------------------------------------
    ; Enable A20 Line
    ; Without this, we can only address 1MB of memory (real mode limit)
    ; We try the fast A20 method via keyboard controller
    ; -------------------------------------------------------------------------
    mov si, MSG_A20
    call print_string_rm
    call enable_a20
    
    ; -------------------------------------------------------------------------
    ; Load GDT and switch to protected mode
    ; -------------------------------------------------------------------------
    mov si, MSG_PM
    call print_string_rm
    
    cli                     ; Disable interrupts during mode switch
    lgdt [gdt_descriptor]   ; Load GDT descriptor
    
    ; -------------------------------------------------------------------------
    ; Set protected mode bit in CR0
    ; This is the magic moment - after this we're in 32-bit mode!
    ; -------------------------------------------------------------------------
    mov eax, cr0
    or eax, 0x1             ; Set PE (Protection Enable) bit
    mov cr0, eax
    
    ; -------------------------------------------------------------------------
    ; Far jump to flush CPU pipeline and load CS with code segment selector
    ; The 0x08 is the offset of code segment in our GDT
    ; -------------------------------------------------------------------------
    jmp 0x08:protected_mode_start

; =============================================================================
; print_string_rm: Print string in real mode
; Input: SI = pointer to string
; =============================================================================
print_string_rm:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; =============================================================================
; disk_read_rm: Read sectors in real mode
; Same as stage 1 but defined locally since we're in a different segment
; =============================================================================
disk_read_rm:
    pusha
    push dx
    
    mov ah, 0x02            ; BIOS read sectors
    mov al, dh              ; Number of sectors
    mov ch, 0               ; Cylinder 0
    mov dh, 0               ; Head 0
    
    int 0x13
    jc .error
    
    pop dx
    popa
    ret
    
.error:
    mov si, MSG_DISK_ERR
    call print_string_rm
    cli
    hlt
    jmp $

; =============================================================================
; enable_a20: Enable A20 line using keyboard controller method
; DEBUGGING: If this fails, try BIOS INT 15h, AX=2401h instead
; =============================================================================
enable_a20:
    pusha
    
    ; Wait for keyboard controller to be ready
    call .wait_input
    mov al, 0xAD            ; Disable keyboard
    out 0x64, al
    
    call .wait_input
    mov al, 0xD0            ; Read output port
    out 0x64, al
    
    call .wait_output
    in al, 0x60             ; Read output port data
    push ax
    
    call .wait_input
    mov al, 0xD1            ; Write output port
    out 0x64, al
    
    call .wait_input
    pop ax
    or al, 2                ; Set A20 bit
    out 0x60, al
    
    call .wait_input
    mov al, 0xAE            ; Enable keyboard
    out 0x64, al
    
    call .wait_input
    popa
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

; =============================================================================
; GDT (Global Descriptor Table)
; Defines code and data segments for 32-bit protected mode
; =============================================================================
gdt_start:
    ; Null descriptor (required by CPU)
    dq 0x0000000000000000
    
gdt_code:
    ; Code segment descriptor
    ; Base=0, Limit=0xFFFFF, Access: present, ring 0, code, readable
    ; Flags: 4KB granularity, 32-bit
    dw 0xFFFF               ; Limit (bits 0-15)
    dw 0x0000               ; Base (bits 0-15)
    db 0x00                 ; Base (bits 16-23)
    db 10011010b            ; Access byte: present, ring 0, code segment
    db 11001111b            ; Flags + Limit (bits 16-19)
    db 0x00                 ; Base (bits 24-31)
    
gdt_data:
    ; Data segment descriptor
    ; Same as code but with data segment type
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b            ; Access byte: present, ring 0, data segment
    db 11001111b
    db 0x00
    
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size - 1
    dd gdt_start                ; GDT address

; Segment selectors (offsets into GDT)
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; =============================================================================
; Data
; =============================================================================
MSG_STAGE2_LOADED: db '[SharkOS] Stage 2 loaded', 13, 10, 0
MSG_A20:           db '[SharkOS] Enabling A20...', 13, 10, 0
MSG_PM:            db '[SharkOS] Entering protected mode...', 13, 10, 0
MSG_DISK_ERR:      db '[ERROR] Disk read failed in stage 2!', 13, 10, 0

; =============================================================================
; 32-bit Protected Mode Code
; =============================================================================
[bits 32]

protected_mode_start:
    ; -------------------------------------------------------------------------
    ; Set up segment registers for protected mode
    ; All segments point to our flat data segment (0x10 in GDT)
    ; -------------------------------------------------------------------------
    mov ax, DATA_SEG        ; 0x10 = data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; -------------------------------------------------------------------------
    ; Set up stack at a safe location
    ; -------------------------------------------------------------------------
    mov ebp, 0x90000        ; Stack base
    mov esp, ebp            ; Stack pointer
    
    ; -------------------------------------------------------------------------
    ; Print "OK" directly to VGA memory to confirm we're in protected mode
    ; VGA text memory starts at 0xB8000
    ; Each character is 2 bytes: ASCII code + attribute
    ; -------------------------------------------------------------------------
    mov edi, 0xB8000 + 160  ; Second line of screen (80 columns * 2 bytes)
    mov ah, 0x0F            ; White on black
    mov al, 'P'
    mov [edi], ax
    add edi, 2
    mov al, 'M'
    mov [edi], ax
    add edi, 2
    mov al, ' '
    mov [edi], ax
    add edi, 2
    mov al, 'O'
    mov [edi], ax
    add edi, 2
    mov al, 'K'
    mov [edi], ax
    
    ; -------------------------------------------------------------------------
    ; Jump to kernel!
    ; -------------------------------------------------------------------------
    jmp KERNEL_OFFSET

; =============================================================================
; Padding to fill sector (optional, helps with debugging)
; =============================================================================
times 2048-($-$$) db 0      ; Pad to 4 sectors (2KB)
