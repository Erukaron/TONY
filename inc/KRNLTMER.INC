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
PIT_FREQUENCY                    equ 1193182     ; oscillation frequency (Hz) of the 8253 PIT
TIMER_FREQUENCY                  equ 1165        ; system timer frequency (Hz) (1193182 / 2^3)
TIMER_DEFAULT_FREQUENCY          equ 18          ; 18, 2 Hz
TIMER_TICKS_DEFAULT_FRQ          equ TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY ; Number of ticks for one tick in default frequency
TIMER_DIVISOR                    equ PIT_FREQUENCY / TIMER_FREQUENCY ; used to configure the PIT
TIMER_TICKS_TO_MS                equ 1 ; ,165216797    0.001 / (1 / TIMER_FREQUENCY)
; used for correction while sleeping
TIMER_TICKS_TO_MS_REMAINDER_1    equ 1
TIMER_TICKS_TO_MS_REMAINDER_10   equ 6
TIMER_TICKS_TO_MS_REMAINDER_100  equ 5
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
timer_tick_count_lo dw 0
timer_tick_count_hi dw 0
timer_bios_tick_count dw TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY
timer_bios_int_offset dw 0
timer_bios_int_segment dw 0
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

    mov [cs:timer_bios_int_offset], dx
    mov [cs:timer_bios_int_segment], bp

    cli 

    mov al, 0b00110100                 ; PIT channel 0
    out 43h, al                 ; select channel

    mov ax, TIMER_DIVISOR
    out 40h, al    ;send low byte
    mov al, ah
    out 40h, al    ;send high byte

    ; reset timer
    mov word [cs:timer_tick_count_lo], 0
    mov word [cs:timer_tick_count_hi], 0
    mov word [cs:timer_bios_tick_count], TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY

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

    ;cmp byte [cs:timer_breakpoint_byte], 1
    ;jnz .no_break
    ;    xchg bx, bx
    .no_break:

    mov ax, word [cs:timer_tick_count_lo]
    add ax, 1
    jnc .no_inc_hi
        mov ax, 0
        inc word [cs:timer_tick_count_hi]

    .no_inc_hi:

    mov word [cs:timer_tick_count_lo], ax

    cmp word [cs:timer_bios_tick_count], TIMER_FREQUENCY / TIMER_DEFAULT_FREQUENCY
    jb .no_int_yet
        mov byte [cs:timer_bios_tick_count], 0
        
        ; Bios interrupt -> Reads cmos registers and sends eoi
        pushf
        push cs
        push .done
        ; Load interrupt location into the stack
        push word [cs:timer_bios_int_segment]
        push word [cs:timer_bios_int_offset]
        ; Jump to bios interrupt location via stack
        retf

    .no_int_yet:
        call kernel_acknowledge_hardware_interrupt

    .done:
        pop ax
        iret

    timer_breakpoint_byte db 0
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
    pusha
        mov bx, ax

        mov cx, TIMER_TICKS_TO_MS
        mul cx ; dx:ax -> Number of ticks to wait

        add ax, [cs:timer_tick_count_lo]
        jnc .no_increase_dx
            inc dx

        .no_increase_dx:
        add dx, [cs:timer_tick_count_hi]

        sti ; allow time keeping while calculating correction

        mov byte [cs:.correction_factor], TIMER_TICKS_TO_MS_REMAINDER_1
        mov word [cs:.correction_divisor], 10
        call .correct
        mov byte [cs:.correction_factor], TIMER_TICKS_TO_MS_REMAINDER_10
        mov word [cs:.correction_divisor], 100
        call .correct
        mov byte [cs:.correction_factor], TIMER_TICKS_TO_MS_REMAINDER_100
        mov word [cs:.correction_divisor], 1000
        call .correct

        ; Wait until timer ticks are equal or greater than the calculated tick count
        .wait:
            sti
            nop ; nop a few times so the interrupt can get handled
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

            cli

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

    .correct:
        push bx
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
                mov cx, [cs:.correction_divisor]
                div cx

                ; 3 * 192 = 576 ticks
                xor cx, cx
                mov cl, [cs:.correction_factor]
                mul cx

                mov bx, ax
                mov cx, dx
            pop dx
            pop ax

            ; Continuing the example from above:
            ; 24000 ticks + 576 ticks = 24576 ticks / 8192 = 3s
            add ax, bx
            jnc .no_increase_dx_2
                inc dx

            .no_increase_dx_2:
            add dx, cx

        pop bx

        ret

    .correction_factor db 0
    .correction_divisor dw 0
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
; Get random number (byte)
;
; input:
;   none
; output:
;   AL -> Random number
;-------------------------------------------------------------------------------
timer_create_random_number:
    sti

    push bx
    push cx
    push bp
    mov bx, ax

    xor ax, cx
    rcr ax, cl

    xor al, [cs:keyboard_key_buffer]
    mov cl, [cs:keyboard_key_buffer_entries]
    inc cl
    mul cl ; randomizer, if user entered something
    mov cl, al

    xor al, [cs:timer_tick_count_lo]
    rcr ax, cl

    add cl, 251 ; add a prime number
    mul cl

    xor ax, si
    xor cl, al
    rcr ax, cl
    xor ax, di
    xor cl, al
    rcr ax, cl
    xor ax, bp
    xor cl, bl
    rcr ax, cl
    xor ax, sp
    xor cl, bh
    rcr ax, cl
    xor ax, [ds:si]
    xor cl, ah
    rcr ax, cl
    xor ax, [ds:di]
    xor cl, al
    rcr ax, cl
    xor ax, [es:si]
    xor cl, al
    rcr ax, cl
    xor ax, [es:di]
    xor cl, al
    rcr ax, cl
    mov bp, sp
    add bp, 10
    xor ax, [ss:bp]
    xor cl, al
    rcr ax, cl

    xor al, ah

    mov ah, bh

    xor [cs:.timer_seed], al

    pop bp
    pop cx
    pop bx
    iret

    .timer_seed db 0
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
;    sti ; Allow hardware interrupts to occur while processing software interrupts
;    iret / ret
;-------------------------------------------------------------------------------
