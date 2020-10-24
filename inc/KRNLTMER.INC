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
; TONY Kernel timer module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
PIT_FREQUENCY           equ 1193182     ; oscillation frequency (Hz) of the 8253 PIT
TIMER_FREQUENCY         equ 1000        ; system timer frequency (Hz)
TIMER_DEFAULT_FREQUENCY equ 18
TIMER_TICKS_DEFAULT_FRQ equ TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY ; Number of ticks for one tick in default frequency
TIMER_DIVISOR           equ PIT_FREQUENCY / TIMER_FREQUENCY ; used to configure the PIT
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
timer_tick_count dw 0
timer_rtc_bios_int_offset dw 0
timer_rtc_bios_int_segment dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Initializes the system timer
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
timer_init:
    ; Test multiple of 18,2Hz with PIT

    mov [cs:timer_rtc_bios_int_offset], dx
    mov [cs:timer_rtc_bios_int_segment], bp

    cli 

    ; disable NMI
    xchg bx, bx
    in al, 0x70
    and al, 0x7f
    out 0x70, al
    in al, 0x70

    ; Chip scheint nicht vorhanden zu sein... Wert lesen, verändern, schreiben und lesen
    ; bringt bei beiden Lesevorgängen den selben Wert, dieser sollte aber unterschiedlich sein
    ; Checken ob man Bochs dazu bringen muss RTC/NMI zur Verfügung zu stellen / Was mich daran hindert das zu verwenden

    ; Set freq
    mov al, 0x0a
    out 0x70, al

    in al, 0x71
    or al, 110b
    mov ah, al

    mov al, 0x0a
    out 0x70, al

    mov al, ah
    out 0x71, al

    ; Enable int
    mov al, 0x0b
    out 0x70, al

    in  al, 0x71
    or  al, 01010000b
    ;and al, 11001111b
    mov ah, al

    mov al, 0x0b
    out 0x70, al

    mov al, ah
    out 0x71, al

    mov word [cs:timer_tick_count], 0

    mov al, 0x0c
    out 0x70, al
    in al, 0x71

    mov al, 0x0d
    out 0x70, al
    in al, 0x71

    ; enable NMI
    in al, 0x70
    or al, 0x80
    out 0x70, al

    sti

    inc al
    out 0x70, al
    in al, 0x70

    int 0x70

    ;call kernel_acknowledge_hardware_interrupt_slave

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Called on timer tick
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
timer_callback:
    push ax

    xchg bx, bx
    inc word [cs:timer_tick_count]
    
    ; Bios interrupt
    ;sti

    ;pushf
    ;push cs
    ;push .next_inst_after_int
    ; Load interrupt location into the stack
    ;push word [cs:timer_rtc_bios_int_segment]
    ;push word [cs:timer_rtc_bios_int_offset]
    ; Jump to stack
    ;retf

    .next_inst_after_int:
    ; read c register 
    cli
    ;mov al, 0x0c
    ;out 0x70, al
    ;in al, 0x71
    sti

    pop ax
    call kernel_acknowledge_hardware_interrupt_slave
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Returns the number of timer ticks (each tick is one milli second)
;
; input: 
;   none
;
; output:
;   timer ticks AX
;-------------------------------------------------------------------------------
timer_get_ticks:
    mov ax, [cs:timer_tick_count]
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Description
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
;name:
;    iret / ret
;-------------------------------------------------------------------------------
