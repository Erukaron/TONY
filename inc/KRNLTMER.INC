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
RTC_TIMER_FREQ_DIVIDER                    equ 3 ; 3 = 8192 Hz | 4 = 4096 Hz | ... | 15 = 2 Hz
RTC_TIMER_TICKS_TO_MS                     equ 8 ; ~8 ticks = 1ms
RTC_TIMER_TICKS_TO_MS_REMAINDER           equ 192 ; 8,192 ticks = 1 ms
RTC_TIMER_TICKS_TO_MS_REMAINDER_DIVIDER   equ 1000
RTC_TIMER_CRYSTAL_FREQ                    equ 32768 ; Hz
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
timer_tick_count_lo dw 0
timer_tick_count_hi dw 0
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
    pusha 

    mov [cs:timer_rtc_bios_int_offset], dx
    mov [cs:timer_rtc_bios_int_segment], bp

    cli 

    ; Unmask interrupt...
    mov bl, 8 ; rtc
    call kernel_enable_hardware_interrupt

    ; Start periodic interrupt
    mov al, 0x8b 
    out 0x70, al ; select register b and disable NMI

    in al, 0x71 ; read current content of register b
    mov ah, al

    mov al, 0x8b
    out 0x70, al ; reset register b

    or ah, 0x40
    mov al, ah
    out 0x71, al ; Enable periodic interrupt

    ; Set freq
    mov bl, RTC_TIMER_FREQ_DIVIDER
    and bl, 0x0f

    mov al, 0x8a ; disable NMI and select reg A
    out 0x70, al

    in al, 0x71
    mov ah, al
    and ah, 0xf0 ; erase current divisor settings
    or ah, bl

    mov al, 0x8a
    out 0x70, al

    mov al, ah
    out 0x71, al

    ; read register c to allow for new interrupts
    mov al, 0x0c
    out 0x70, al
    in al, 0x71

    ; Also set to register d according to https://www.compuphase.com/int70.txt
    mov al, 0x0d
    out 0x70, al
    in al, 0x71

    ; reset timer
    mov word [cs:timer_tick_count_lo], 0
    mov word [cs:timer_tick_count_hi], 0

    sti
    popa
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

    mov ax, word [cs:timer_tick_count_lo]
    add ax, 1
    jnc .no_inc_hi
        mov ax, 0
        inc word [cs:timer_tick_count_hi]

    .no_inc_hi:

    mov word [cs:timer_tick_count_lo], ax
    
    ; Bios interrupt -> Reads cmos registers and sends eoi
    pushf
    push cs
    push .next_inst_after_int
    ; Load interrupt location into the stack
    push word [cs:timer_rtc_bios_int_segment]
    push word [cs:timer_rtc_bios_int_offset]
    ; Jump to bios interrupt location via stack
    retf

    .next_inst_after_int:

    pop ax
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Returns the number of timer ticks (each tick is 122µs)
;
; input: 
;   none
;
; output:
;   AX -> timer ticks hi
;   DX -> timer ticks lo
;-------------------------------------------------------------------------------
timer_get_ticks:
    mov ax, [cs:timer_tick_count_hi]
    mov dx, [cs:timer_tick_count_lo]
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sleep millis
;
; input: 
;   AX -> milli seconds
;
; output:
;   none
;-------------------------------------------------------------------------------
timer_sleep_millis:
    sti ; Allow interrupts, otherwise there will be no counting

    pusha
        mov bx, ax

        xor cx, cx
        mov cl, RTC_TIMER_TICKS_TO_MS
        mul cx ; dx:ax -> Number of ticks to wait

        add ax, [cs:timer_tick_count_lo]
        jnc .no_increase_dx
            inc dx

        .no_increase_dx:
        add dx, [cs:timer_tick_count_hi]

        push ax
        push dx
            ; calculate error from RTC_TIMER_TICKS_TO_MS
            ; Example:
            ; Input is: 3000ms
            ; The code above alone would sleep for: 3000*8 = 240000 ticks
            ; 24000 ticks => 24000/8192 = 2,93s
            ; With the code below:
            ; 3000 / 1000 = 3
            xor dx, dx
            mov ax, bx
            xor cx, cx
            mov cx, RTC_TIMER_TICKS_TO_MS_REMAINDER_DIVIDER
            div cx

            ; 3 * 192 = 576 ticks
            xor cx, cx
            mov cl, RTC_TIMER_TICKS_TO_MS_REMAINDER
            mul cx

            mov bx, ax
            mov cx, dx
        pop dx
        pop ax

        ; Continuing the example from above:
        ; 24000 ticks + 576 ticks = 24576 ticks / 8192 = 3s
        ; => The correction does only work for sleep periods of 1000ms or more
        add ax, bx
        jnc .no_increase_dx_2
            inc dx

        .no_increase_dx_2:
        add dx, cx

        ; Wait until timer ticks are equal or greater than the calculated tick count
        .wait:
            cmp [cs:timer_tick_count_hi], dx
            ja .done
            jb .wait

            ; high ticks are equal
            cmp [cs:timer_tick_count_lo], ax
            ja .done
            jb .wait

    .done:
        popa

        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sleep
;
; input: 
;   AX -> seconds
;
; output:
;   none
;-------------------------------------------------------------------------------
timer_sleep_seconds:
    push cx
    push ax

    mov cx, ax
    mov ax, 1000
    .wait_1000ms:
        int 0x81 ; timer_sleep_millis
        loop .wait_1000ms

    pop ax
    pop cx
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
