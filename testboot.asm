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
; Test bootloader
;-------------------------------------------------------------------------------
bits 16
org 0
jmp start
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Loader
;--------------------------------------------------------------------------------------
start:
    cli ; Disable interrups while segments are set up

    mov ax, 0
    mov ds, ax
    mov es, ax

    mov ax, 0x1000
    mov ss, ax
    xor sp, sp

    sti

    mov si, boot_msg
    call print_ln

    ; Install rtc interrupt
    ; each ivt entry is 32 bit long [offset][segment]
    xor ax, ax
    mov cl, 4
    mov ah, 0x70 ; rtc interrupt vector
    mul cl

    mov si, ax

    mov dx, [ds:0x1c0] ; Old timer interrupt
    mov bp, [ds:0x1c2]
    mov [rtc_int_segment], bp
    mov [rtc_int_offset], dx

    cli ; disable interrupts while setting up rtc

    mov [ds:si], word interrupt_call
    mov [ds:si + 2], cs

    ; Test if rtc is accessible
    xchg bx, bx
    in al, 0x70
    mov al, 0x0a
    out 0x70, al
    in al, 0x70

    sti

    .wait_for_interrupt:
        nop
    jmp .wait_for_interrupt
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; interrupt routine
;--------------------------------------------------------------------------------------
interrupt_call:
    xchg bx, bx
    iret
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Prints the null terminated string in DS:SI
;--------------------------------------------------------------------------------------
print:
    pusha 

    .exec:
        lodsb
        cmp al, 0
        je .done

        mov ah, 0x0e
        int 0x10

        jmp .exec

    .done:
        popa
        ret
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Prints the null terminated string in DS:SI
;--------------------------------------------------------------------------------------
print_ln:
    call print

    push si

    mov si, .crlf
    call print

    pop si

    ret

    .crlf db 0x0a, 0x0d, 0
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Halts the system on fatal error
;--------------------------------------------------------------------------------------
halt:
    mov si, halt_msg
    call print 

    int 0x18 ; No bootable device...
    cli
    hlt
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Data
;--------------------------------------------------------------------------------------
boot_msg db "TONY: TEST-BOOT", 0
halt_msg db "ERROR", 0
rtc_int_segment dw 0
rtc_int_offset  dw 0
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Boot-Magic
;--------------------------------------------------------------------------------------
times 510-($-$$) nop
db 0x55
db 0xaa
;---------------------------------