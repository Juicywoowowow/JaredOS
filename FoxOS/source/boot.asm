; =============================================================================
; boot.asm - FoxOS Bootloader
; =============================================================================
;
; This is the first code that runs when the computer boots. The BIOS loads
; this 512-byte sector from the first sector of the disk into memory at
; 0x7C00 and jumps to it.
;
; Our bootloader does the following:
;   1. Set up segment registers and stack
;   2. Load the kernel from disk into memory
;   3. Switch from 16-bit Real Mode to 32-bit Protected Mode
;   4. Jump to the kernel
;
; DEBUGGING TIPS:
;   - If nothing appears on screen, the boot sector may not be loading
;   - Use QEMU's -d int flag to see interrupts
;   - Check that the boot signature (0xAA55) is at bytes 510-511
;
; =============================================================================

[BITS 16]                       ; We start in 16-bit Real Mode
[ORG 0x7C00]                    ; BIOS loads us at this address

; =============================================================================
; SECTION 1: Entry Point
; =============================================================================

boot_start:
    ; Clear interrupts during setup
    cli
    
    ; Set up segment registers
    ; In Real Mode, memory address = segment * 16 + offset
    xor ax, ax                  ; AX = 0
    mov ds, ax                  ; Data Segment = 0
    mov es, ax                  ; Extra Segment = 0
    mov ss, ax                  ; Stack Segment = 0
    mov sp, 0x7C00              ; Stack grows down from bootloader
    
    ; Enable interrupts again
    sti
    
    ; Save boot drive number (BIOS passes it in DL)
    mov [BOOT_DRIVE], dl

    ; Print loading message
    mov si, MSG_LOADING
    call print_string

; =============================================================================
; SECTION 2: Load Kernel from Disk
; =============================================================================
; We use BIOS interrupt 0x13 to read sectors from disk.
; The kernel is loaded starting at 0x10000 (64KB mark).

load_kernel:
    mov bx, KERNEL_OFFSET       ; Load kernel to this address
    mov dh, 32                  ; Number of sectors to read (16KB)
    mov dl, [BOOT_DRIVE]        ; Drive number
    call disk_load
    
    ; Print success message
    mov si, MSG_LOADED
    call print_string

; =============================================================================
; SECTION 3: Switch to Protected Mode
; =============================================================================
; Protected mode gives us:
;   - 32-bit registers and addressing
;   - Memory protection
;   - Access to all 4GB of address space

switch_to_pm:
    cli                         ; Disable interrupts during switch
    lgdt [gdt_descriptor]       ; Load GDT descriptor
    
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Far jump to flush CPU pipeline and load CS with 32-bit code segment
    jmp CODE_SEG:init_pm

; =============================================================================
; SECTION 4: 16-bit Helper Functions
; =============================================================================

; -----------------------------------------------------------------------------
; print_string - Print null-terminated string to screen
; Input: SI = pointer to string
; -----------------------------------------------------------------------------
print_string:
    pusha
.loop:
    lodsb                       ; Load byte from SI into AL, increment SI
    or al, al                   ; Check if null terminator
    jz .done
    mov ah, 0x0E                ; BIOS teletype function
    int 0x10                    ; Call BIOS video interrupt
    jmp .loop
.done:
    popa
    ret

; -----------------------------------------------------------------------------
; disk_load - Load sectors from disk using BIOS int 0x13
; Input: DH = number of sectors, DL = drive number, BX = destination offset
; -----------------------------------------------------------------------------
disk_load:
    pusha
    push dx                     ; Save DX for later comparison
    
    mov ah, 0x02                ; BIOS read sectors function
    mov al, dh                  ; Number of sectors to read
    mov cl, 0x02                ; Start from sector 2 (sector 1 is boot sector)
    mov ch, 0x00                ; Cylinder 0
    mov dh, 0x00                ; Head 0
    ; DL already contains drive number
    
    int 0x13                    ; Call BIOS disk interrupt
    jc disk_error               ; Jump if carry flag set (error)
    
    pop dx                      ; Restore original DX
    cmp al, dh                  ; Check if we read expected sectors
    jne sectors_error
    
    popa
    ret

disk_error:
    mov si, MSG_DISK_ERR
    call print_string
    jmp $                       ; Infinite loop

sectors_error:
    mov si, MSG_SECTOR_ERR
    call print_string
    jmp $                       ; Infinite loop

; =============================================================================
; SECTION 5: Global Descriptor Table (GDT)
; =============================================================================
; The GDT defines memory segments for protected mode.
; We use a flat memory model with overlapping code and data segments.

gdt_start:
    ; Null descriptor (required, index 0)
    dq 0x0000000000000000

gdt_code:
    ; Code segment descriptor
    ; Base=0, Limit=0xFFFFF, Access=0x9A, Flags=0xCF
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0x0000                   ; Base (bits 0-15)
    db 0x00                     ; Base (bits 16-23)
    db 10011010b                ; Access byte: Present, Ring 0, Code, Executable, Readable
    db 11001111b                ; Flags: 4KB granularity, 32-bit, Limit (bits 16-19)
    db 0x00                     ; Base (bits 24-31)

gdt_data:
    ; Data segment descriptor
    ; Base=0, Limit=0xFFFFF, Access=0x92, Flags=0xCF
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0x0000                   ; Base (bits 0-15)
    db 0x00                     ; Base (bits 16-23)
    db 10010010b                ; Access byte: Present, Ring 0, Data, Writable
    db 11001111b                ; Flags: 4KB granularity, 32-bit, Limit (bits 16-19)
    db 0x00                     ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size (limit)
    dd gdt_start                ; GDT base address

; Segment selector constants
CODE_SEG equ gdt_code - gdt_start   ; 0x08
DATA_SEG equ gdt_data - gdt_start   ; 0x10

; =============================================================================
; SECTION 6: 32-bit Protected Mode Entry
; =============================================================================

[BITS 32]

init_pm:
    ; Update segment registers with data segment selector
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack at 0x90000
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Jump to kernel entry point
    call KERNEL_OFFSET
    
    ; If kernel returns (it shouldn't), hang
    jmp $

; =============================================================================
; SECTION 7: Data Section
; =============================================================================

BOOT_DRIVE:     db 0
KERNEL_OFFSET   equ 0x10000     ; Where we load the kernel

MSG_LOADING:    db "FoxOS Bootloader...", 13, 10, 0
MSG_LOADED:     db "Kernel loaded, switching to PM...", 13, 10, 0
MSG_DISK_ERR:   db "Disk read error!", 13, 10, 0
MSG_SECTOR_ERR: db "Sector count mismatch!", 13, 10, 0

; =============================================================================
; SECTION 8: Boot Sector Padding and Signature
; =============================================================================

times 510 - ($ - $$) db 0       ; Pad with zeros to 510 bytes
dw 0xAA55                       ; Boot sector signature (must be last 2 bytes)
