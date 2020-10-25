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
TIMER_FREQUENCY         equ 1820        ; system timer frequency (Hz)
TIMER_DEFAULT_FREQUENCY equ 18          ; 18, 2 Hz
TIMER_TICKS_DEFAULT_FRQ equ TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY ; Number of ticks for one tick in default frequency
TIMER_DIVISOR           equ PIT_FREQUENCY / TIMER_FREQUENCY ; used to configure the PIT
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
timer_tick_count dw 0
timer_bios_tick_count db 0
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
    mov [cs:timer_rtc_bios_int_offset], dx
    mov [cs:timer_rtc_bios_int_segment], bp

    cli 

    mov word [cs:timer_tick_count], 0

    mov al, 36h                 ; PIT channel 0
    out 43h, al                 ; select channel

    mov ax, TIMER_DIVISOR
    out 40h, al    ;send low byte
    mov al, ah
    out 40h, al    ;send high byte

    sti

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

    inc word [cs:timer_tick_count]
    inc byte [cs:timer_bios_tick_count]
    
    xchg bx, bx
    cmp byte [cs:timer_bios_tick_count], TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY
    jne .next_inst_after_int

    mov byte [cs:timer_bios_tick_count], 0

    ; Bios interrupt
    pushf
    push cs
    push .next_inst_after_int
    ; Load interrupt location into the stack
    push word [cs:timer_rtc_bios_int_segment]
    push word [cs:timer_rtc_bios_int_offset]
    ; Jump to stack
    retf

    .next_inst_after_int:
    ;sti

    pop ax
    call kernel_acknowledge_hardware_interrupt
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
