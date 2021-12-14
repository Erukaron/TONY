;-------------------------------------------------------------------------------
; MIT License
;
; Copyright (c) 2020 Erukaron
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%include "COM_HEAD.INC"
jmp MAIN
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
%define KERNEL_IMAGE_RMODE_SEGMENT 0x4000
%define KERNEL_IMAGE_PMODE_BASE 0x100000 ; 1 MiB
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
kernel32_image_size dd 0
;-------------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Enables the a20 line via the keyboard controller
;
;Port   Read/Write  Descripton
;0x60   Read    Read Input Buffer
;0x60   Write   Write Output Buffer
;0x64   Read    Read Status Register
;0x64   Write   Send Command to controller
;
;
;    Bit 0: Output Buffer Status
;        0: Output buffer empty, dont read yet
;        1: Output buffer full, please read me :)
;    Bit 1: Input Buffer Status
;        0: Input buffer empty, can be written
;        1: Input buffer full, dont write yet
;    Bit 2: System flag
;        0: Set after power on reset
;        1: Set after successfull completion of the keyboard controllers self-test (Basic Assurance Test, BAT)
;    Bit 3: Command Data
;        0: Last write to input buffer was data (via port 0x60)
;        1: Last write to input buffer was a command (via port 0x64)
;    Bit 4: Keyboard Locked
;        0: Locked
;        1: Not locked
;    Bit 5: Auxiliary Output buffer full
;        PS/2 Systems:
;            0: Determins if read from port 0x60 is valid If valid, 0=Keyboard data
;            1: Mouse data, only if you can read from port 0x60
;        AT Systems:
;            0: OK flag
;            1: Timeout on transmission from keyboard controller to keyboard. This may indicate no keyboard is present.
;    Bit 6: Timeout
;        0: OK flag
;        1: Timeout
;        PS/2:
;            General Timeout
;        AT:
;            Timeout on transmission from keyboard to keyboard controller. Possibly parity error (In which case both bits 6 and 7 are set)
;    Bit 7: Parity error
;        0: OK flag, no error
;        1: Parity error with last byte
;
;Keyboard Command   Descripton
;0x20   Read Keyboard Controller Command Byte
;0x60   Write Keyboard Controller Command Byte
;0xAA   Self Test
;0xAB   Interface Test
;0xAD   Disable Keyboard
;0xAE   Enable Keyboard
;0xC0   Read Input Port
;0xD0   Read Output Port
;0xD1   Write Output Port
;0xDD   Enable A20 Address Line
;0xDF   Disable A20 Address Line
;0xE0   Read Test Inputs
;0xFE   System Reset
;Mouse Command  Descripton
;0xA7   Disable Mouse Port
;0xA8   Enable Mouse Port
;0xA9   Test Mouse Port
;0xD4   Write to mouse
;
; Output port byte:
;    Bit 0: System Reset
;        0: Reset computer
;        1: Normal operation
;    Bit 1: A20
;        0: Disabled
;        1: Enabled
;    Bit 2-3: Undefined
;    Bit 4: Input buffer full
;    Bit 5: Output Buffer Empty
;    Bit 6: Keyboard Clock
;        0: High-Z
;        1: Pull Clock Low
;    Bit 6: Keyboard Data
;        0: High-Z
;        1: Pull Data Low
;-----------------------------------------------------------------------------
enable_a20_keyboard:
    cli
    pusha

    call .wait_input

    ; Disable keyboard
    mov al, 0xad
    out 0x64, al
    call .wait_input

    ; Get output port data
    mov al, 0xd0
    out 0x64, al
    call .wait_output

    ; Read the input buffer and store it -> data from the output port
    in al, 0x60
    push eax
    call .wait_input

    ; Prepare for write output port
    mov al, 0xd1
    out 0x64, al
    call .wait_input

    ; Enable a20
    pop eax
    or al, 2
    out 0x60, al
    call .wait_input

    ; Enable keyboard
    mov al, 0xae
    out 0x64, al
    call .wait_input

    popa
    sti
    ret


    .wait_input:
        in al, 0x64 ; Read status register of keyboard
        test al, 2 ; Input buffer status
        jnz .wait_input ; Buffer not empty, continue waiting
        ret

    .wait_output:
        in al, 0x64
        test al, 1 ; Output buffer status
        jz .wait_output
        ret
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Enables the a20 line via BIOS functionallity
;-----------------------------------------------------------------------------
enable_a20_bios:
    pusha
    mov ax, 0x2402
    int 0x15
    popa
    ret
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Enables the a20 line via the system control port a
; Some systems, including MCA and EISA we can control A20 from the system control port I/O 0x92. The exact details and functions of port 0x92 vary greatly by manufacturer. There are several bits that are commonly used though:
;
;    Bit 0 - Setting to 1 causes a fast reset (Used to switch back to real mode)
;    Bit 1 - 0: disable A20; 1: enable A20
;    Bit 2 - Manufacturer defined
;    Bit 3 - power on password bytes (CMOS bytes 0x38-0x3f or 0x36-0x3f). 0: accessible, 1: inaccessible
;    Bits 4-5 - Manufacturer defined
;    Bits 6-7 - 00: HDD activity LED off; any other value is "on"
;-----------------------------------------------------------------------------
enable_a20_sys_cp_a:
    pusha 
    mov al, 2
    out 0x92, al
    popa
    ret
;-----------------------------------------------------------------------------

%define NULL_DESCRIPTOR 0
%define CODE_DESCRIPTOR 0x08
%define DATA_DESCRIPTOR 0x10
%define USER_CODE 0x18
%define USER_DATA 0x20
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Installs the gdt
;-----------------------------------------------------------------------------
setup_gdt:
    cli 
    pusha 
    lgdt [gdtr] ; load GDT into GDTR
    sti 
    popa 
    ret
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Actual table
;-----------------------------------------------------------------------------
gdt_data:
; offset 0x00
    dw 0
    dw 0
    dw 0
    dw 0

; offset 0x08 - kernel code
    dw 0xffff       ; segment limit low
    dw 0            ; base low
    db 0            ; base mid
    db 10011010b    ; access byte 
    db 11001111b    ; 4 bit flags | 4 bit limit high
    db 0            ; base high

; offset 0x10 - kernel data
    dw 0xffff       ; segment limit low
    dw 0            ; base low
    db 0            ; base mid
    db 10010010b    ; access byte 
    db 11001111b    ; 4 bit flags | 4 bit limit high
    db 0            ; base high

; offset 0x18 - user code
    dw 0xffff       ; segment limit low
    dw 0            ; base low
    db 0            ; base mid
    db 11111010b    ; access byte 
    db 11001111b    ; 4 bit flags | 4 bit limit high
    db 0            ; base high

; offset 0x20 - user data
    dw 0xffff       ; segment limit low
    dw 0            ; base low
    db 0            ; base mid
    db 11110010b    ; access byte 
    db 11001111b    ; 4 bit flags | 4 bit limit high
    db 0            ; base high

end_of_gdt:
gdtr: 
    dw end_of_gdt - gdt_data - 1 ; limit (Size of GDT)
    dd gdt_data ; base of GDT
;-----------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Entrypoint
;-------------------------------------------------------------------------------
MAIN:
    cli
    ; Addressspace 0x0500 - 0xffff reachable without segment registers --> Zero them for compatiblity with gdt
    mov ax, 0
    mov fs, ax
    mov gs, ax

    ; Enable a20
    call enable_a20_keyboard

    ; Setup the gdt
    call setup_gdt

    ; IF 32-BIT KERNEL GETS TOO BIG -> THIS WON'T WORK, BECAUSE OF MEMORY NOT BEING ABLE TO BE OVERWRITTEN AFTER 0x7ffff OR LATER
    push KERNEL_IMAGE_RMODE_SEGMENT
    pop es
    mov di, 0
    mov si, argv
    int 0xc0

    jc error ; on error

    xchg bx, bx ; Check if endian is correct
    mov [cs:kernel32_image_size], dx
    mov [cs:kernel32_image_size + 2], bx

    ; Enter protected mode
    cli 
    xor eax, eax
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    ; DO NOT REENABLE INTERRUPTS!

    jmp clear_prefetch_queue
    nop
    nop
    clear_prefetch_queue:

    jmp CODE_DESCRIPTOR: dword switch_kernel ; Load cs with the code descriptor

    error:
        ret
;-------------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Switches kernel (32 bit)
; DO ONLY USE 32 BIT PROTECTED MODE COMPATIBLE MODE BELOW!
;-----------------------------------------------------------------------------
bits 32
switch_kernel:
    ; Set all data segments
    mov ax, DATA_DESCRIPTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x8ffff

    ; Relocate kernel to 0x100000
    mov eax, dword [kernel32_image_size]
    mov ebx, 4 ; double word move incoming...
    div ebx
    cld
    mov esi, KERNEL_IMAGE_RMODE_SEGMENT
    shl esi, 4
    mov edi, KERNEL_IMAGE_PMODE_BASE
    mov ecx, eax
    rep movsd ; copy every byte of the image (as double word)

    call CODE_DESCRIPTOR: dword KERNEL_IMAGE_PMODE_BASE

    cli
    hlt
;-----------------------------------------------------------------------------

 