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
; TONY Kernel
;-------------------------------------------------------------------------------
bits 16
%include "COMMON.INC"
org KERNEL_OFFSET
jmp kernel_setup
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
bootdrive db 0
time dw 0
keymap_file db "KBRD_GER.MAP", 0
kernel_config_file db "KERNEL.CFG", 0
font_file db "FONT_3X5.BIN", 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Kernel setup
;-------------------------------------------------------------------------------
kernel_setup:
    ; set data segments
    mov ax, KERNEL_SEGMENT
    mov ds, ax
    mov es, ax

    ; set stack segment
    mov ax, STACK_SEGMENT
    cli
    mov ss, ax
    xor sp, sp
    sti

    ; save bootdrive
    mov [ds:bootdrive], dl

    ; set default video mode
    call display_restore_default_kernel_video_mode

    ; timer needs to be initialized first
    call install_timer
    call install_sound
    call install_display
    call install_keyboard
    call install_memory
    call install_fat
    call install_string
    call install_convert
    call install_mlt

    ; read kernel config file
    ; Todo Adjust keymap file to file from kernel config
    ; Todo Adjust font ; to file from kernel config

    ; read font
    mov si, font_file
    call display_update_font

    ; Now that we have a font file, switch to gfx mode
    mov al, 1 ; gfx mode
    int 0x93 ; Switch video mode

    ; read keyboard file
    mov si, keymap_file
    call keyboard_update_keymap

    ; -------------------Testcode
    .loop1:
    mov cx, 256
    mov ax, cs
    mov es, ax
    mov di, stringdata
    int 0xb1

    mov ds, ax
    mov si, stringdata
    int 0x91
    jmp .loop1

    stringdata times 256 db 0
    ; -------------------Testcode

    ; set default video mode and start mlt
    call display_restore_default_kernel_video_mode
    ; Start mlt
    int 0xf0

    cli
    hlt
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Interrupt installing routines
;-------------------------------------------------------------------------------
install_timer:
    push ds
    mov ax, 0
    mov ds, ax
    mov dx, [ds:0x20] ; Old timer interrupt
    mov bp, [ds:0x22]
    ;call timer_init
    pop ds

    mov al, 0x08 ; Hardware timer
    mov bx, timer_callback
    ;call kernel_install_interrupt

    mov al, 0x80
    mov bx, timer_get_ticks
    call kernel_install_interrupt

    ret

install_sound:
    ret

install_display:
    mov al, 0x90
    mov bx, display_putch
    call kernel_install_interrupt

    mov al, 0x91
    mov bx, display_print
    call kernel_install_interrupt

    mov al, 0x92
    mov bx, display_write
    call kernel_install_interrupt

    mov al, 0x93
    mov bx, display_switch_video_mode
    call kernel_install_interrupt

    mov al, 0x94
    mov bx, display_put_pixel
    call kernel_install_interrupt

    mov al, 0x95
    mov bx, display_clear_screen
    call kernel_install_interrupt

    mov si, timer_installed_msg
    int 0x91

    mov si, sound_installed_msg
    int 0x91

    mov si, display_installed_msg
    int 0x91

    ret

    timer_installed_msg db "Timer interrupts installed.", LINEFEED, RETURN, 0
    sound_installed_msg db "Sound interrupts installed.", LINEFEED, RETURN, 0
    display_installed_msg db "Display interrupts installed.", LINEFEED, RETURN, 0

install_keyboard:
    mov al, 0x09 ; IRQ1 = INT9 is keyboard callback
    mov bx, keyboard_callback
    call kernel_install_interrupt

    mov al, 0xb0
    mov bx, keyboard_getch
    call kernel_install_interrupt

    mov al, 0xb1
    mov bx, keyboard_readln
    call kernel_install_interrupt

    mov al, 0xb2
    mov bx, keyboard_is_function_key_pressed
    call kernel_install_interrupt

    call keyboard_init

    mov si, keyboard_installed_msg
    int 0x91

    ret

    keyboard_installed_msg db "Keyboard interrupts installed.", LINEFEED, RETURN, 0

install_memory:
    mov si, memory_installed_msg
    int 0x91

    ret

    memory_installed_msg db "Memory interrupts installed.", LINEFEED, RETURN, 0

install_fat:
    mov al, 0xc0
    mov bx, fat_load
    call kernel_install_interrupt

    mov al, 0xcf
    mov bx, fat_callback_disk_operation
    call kernel_install_interrupt

    mov dl, [cs:bootdrive]
    mov [cs:fat_current_drive], dl 

    mov si, fat_installed_msg
    int 0x91

    ret

    fat_installed_msg db "FAT interrupts installed.", LINEFEED, RETURN, 0

install_string:
    mov al, 0xd0
    mov bx, str_get_length
    call kernel_install_interrupt

    mov al, 0xd1
    mov bx, str_init
    call kernel_install_interrupt

    mov al, 0xd2
    mov bx, str_contains
    call kernel_install_interrupt

    mov al, 0xd3
    mov bx, str_copy_substring
    call kernel_install_interrupt

    mov al, 0xd4
    mov bx, str_split
    call kernel_install_interrupt

    mov si, str_installed_msg
    int 0x91

    ret

    str_installed_msg db "String interrupts installed.", LINEFEED, RETURN, 0

install_convert:
    mov al, 0xe0
    mov bx, convert_string_case
    call kernel_install_interrupt

    mov al, 0xe1
    mov bx, convert_cluster_lba
    call kernel_install_interrupt

    mov al, 0xe2
    mov bx, convert_lba_chs
    call kernel_install_interrupt

    mov si, convert_installed_msg
    int 0x91

    ret

    convert_installed_msg db "Convert interrupts installed.", LINEFEED, RETURN, 0

install_mlt:
    mov al, 0xf0
    mov bx, mlt_start
    call kernel_install_interrupt

    mov si, mlt_installed_msg
    int 0x91

    ret

    mlt_installed_msg db "MLT interrupts installed.", LINEFEED, RETURN, 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Install an interrupt into the ivt
;
; input: 
;   interrupt vector number AL
;   interrupt handler pointer BX
;
; output:
;   none
;-------------------------------------------------------------------------------
kernel_install_interrupt:
    push ds
    push dx
    push cx
    push si

    mov dx, INTERRUPT_SEGMENT
    mov ds, dx

    ; each ivt entry is 32 bit long [offset][segment]
    mov cl, 4
    mov ah, 0
    mul cl

    mov si, ax

    cli
    mov [ds:si], bx
    mov [ds:si + 2], cs
    sti

    pop si
    pop cx
    pop dx
    pop ds
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Acknowledges all hardware interrupts
;
; input:
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
kernel_acknowledge_hardware_interrupt:
    mov al, 0x20 ; send end of interrupt (eoi)
    out 0x20, al ; send eoi to master pic

    ret
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
; Acknowledges all hardware interrupts
;
; input:
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
kernel_acknowledge_hardware_interrupt_slave:
    mov al, 0x20 ; send end of interrupt (eoi)
    out 0x20, al ; send eoi to master pic
    out 0xa0, al ; send eoi to slave pic

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Includes
;-------------------------------------------------------------------------------
%include "KRNLTMER.INC"
%include "KRNLSND.INC"
%include "KRNLDISP.INC"
%include "KRNLKBRD.INC"
%include "KRNLMEM.INC"
%include "KRNLFAT.INC"
%include "KRNLSTR.INC"
%include "KRNLCVRT.INC"
%include "KRNLMLT.INC"
;-------------------------------------------------------------------------------