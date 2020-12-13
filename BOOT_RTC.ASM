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
; TONY Bootloader
;-------------------------------------------------------------------------------
bits 16
org 0x7c00
jmp start
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Loader
;--------------------------------------------------------------------------------------
start:
    cli ; Disable interrups while segments are set up

    mov ax, 0x0000
    mov ds, ax
    mov es, ax

    mov ax, 0x1000
    mov ss, ax
    xor sp, sp

    ; Set up ivt for 0x70 -> RTC
    push ds
        mov dx, 0x0000
        mov ds, dx

        ; each ivt entry is 32 bit long [offset][segment]
        mov cl, 4
        mov ah, 0
        mov al, 0x70
        mul cl

        mov si, ax

        mov [ds:si], word interrupt_called
        mov [ds:si + 2], cs
    pop ds

    ; Test rtc
    xchg bx, bx

    cli

    ; Unmask interrupt 0x70
    ;http://www.brokenthorn.com/Resources/OSDev16.html
    ;http://ece425web.groups.et.byu.net/stable/labs/InterruptMechanism.html
    ;https://wiki.osdev.org/8259_PIC#Masking
    ;
    in al, 0xa1
    mov ah, 1
    shl ah, 0
    not ah
    and al, ah
    out 0xa1, al

    ; Turn on irq 8
    ;https://wiki.osdev.org/RTC
    ;https://www.compuphase.com/int70.txt
    mov al, 0x8b 
    out 0x70, al ; select register b and disable NMI

    in al, 0x71 ; read current content of register b
    mov ah, al

    mov al, 0x8b
    out 0x70, al ; reset register b

    or ah, 0x40
    mov al, ah
    out 0x71, al ; Enable periodic interrupt

    ; read register c to allow for new interrupts
    mov al, 0x0c
    out 0x70, al
    in al, 0x71

    ; Also set to register d according to https://www.compuphase.com/int70.txt
    mov al, 0x0d
    out 0x70, al
    in al, 0x71

    sti

    .wait_for_int:
        nop
    jmp .wait_for_int
;--------------------------------------------------------------------------------------

interrupt_called:
    xchg bx, bx

    ; read register c to allow for new interrupts
    mov al, 0x0c
    out 0x70, al
    in al, 0x71

    ; Also set to register d according to https://www.compuphase.com/int70.txt
    mov al, 0x0d
    out 0x70, al
    in al, 0x71

    mov al, 0x20 ; send end of interrupt (eoi)
    out 0x20, al ; send eoi to master pic
    out 0xa0, al ; send eoi to slave pic

    iret

;--------------------------------------------------------------------------------------
; Boot-Magic
;--------------------------------------------------------------------------------------
times 510-($-$$) nop
db 0x55
db 0xaa
;---------------------------------