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
; Constants
;-------------------------------------------------------------------------------
PIC1_COMMAND        equ 0x20
PIC1_DATA           equ 0x21
PIC2_COMMAND        equ 0xa0
PIC2_DATA           equ 0xa1
PIC_EOI_MASK        equ 0x20
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
bootdrive db 0
time dw 0
kernel_config_file db "KERNEL.CFG", 0
;-------------------------------------------------------------------------------
; !!! Vars need to have FAT_INPUT_LENGTH + zero terminator size
;-------------------------------------------------------------------------------
keymap_file_initial db "KBRD_EN.MAP ", 0
keymap_file db "KBRD_EN.MAP ", 0
shell_file  db "SHELL.COM   ", 0
font_file   db "FONT_3X5.BIN", 0
shell_cmd_segment dw 0
shell_cmd_offset dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Kernel setup
;-------------------------------------------------------------------------------
kernel_setup:
    cli ; disable interrupts for segment setup

    ; set data segments
    mov ax, KERNEL_SEGMENT
    mov ds, ax
    mov es, ax

    ; set stack segment
    mov ax, STACK_SEGMENT
    mov ss, ax
    xor sp, sp

    ; save bootdrive
    mov [ds:bootdrive], dl
    sti

    ; set default video mode
    call display_restore_default_kernel_video_mode

    ; timer needs to be initialized first
    call install_timer
    call install_math
    call install_display
    call install_sound
    call install_keyboard
    call install_memory
    call install_fat
    call install_string
    call install_convert
    call install_misc

    mov al, 0xf1
    mov bx, _kernel_test_gfx
    call kernel_install_interrupt

    call fat_init

    ; Beep
    xor ax, ax
    ;int 0x88

    ; read default keyboard file
    mov si, keymap_file_initial
    call keyboard_set_intial_keymap

    ; register keyboard hotkeys
    mov al, ASCII_R
    int 0xb4 ; get scancode from ascii char
    ; al set to scancode
    mov ah, 0b111 ; Alt, Ctrl, Shft
    ; bios reset vector -> a jump to this location issues a reset
    mov bx, 0xffff
    mov dx, 0
    int 0xb3

    mov ah, 0
    mov al, SCAN_CODE_F12
    mov bx, cs
    mov dx, oracle
    int 0xb3

    mov ah, 0
    mov al, SCAN_CODE_F11
    mov bx, cs
    mov dx, change_sys_drive_test
    int 0xb3

    mov ah, 0b110 ; Alt, Ctrl
    mov al, SCAN_CODE_DELETE
    ;mov al, SCAN_CODE_M ; -> debug
    mov bx, cs
    mov dx, mlt_call_via_keyboard_event
    int 0xb3

    ; check if config should be read
    mov ah, FUNC_KEY_F7
    int 0xb2
    cmp al, 0x01
    je .skip_config_file
    ; Check if file exists
    mov si, kernel_config_file
    int 0xc1 ; Get file size
    cmp ax, -1 ; file does not exist
    jz .skip_config_file
        ; adjust kernel settings
        call process_config_file
        jmp .after_config_file
    .skip_config_file:
        push si
            mov al, 1
            mov si, .warn_no_kernel_config
            int 0x97
        pop si

        jmp .after_config_file
    .warn_no_kernel_config db "Kernel configuration skipped! Using defaults.", ASCII_CR, ASCII_LF, 0  
    .after_config_file:

    ; read keyboard file, if not default
    push es
        push ds
        pop es

        mov si, keymap_file
        mov di, keymap_file_initial
        mov ah, 0b11
        int 0xd2
        jnc .no_update_keymap
            call keyboard_set_intial_keymap
        .no_update_keymap:
    pop es

    ; check if gfx mode should be enabled
    mov ah, FUNC_KEY_F8
    int 0xb2
    cmp al, 0x01
    je .skip_gfx_mode
    ; check if font file exists
    mov si, font_file
    int 0xc1
    cmp ax, -1
    jz .no_font
        ; read font
        mov al, 0
        mov si, .info_reading_font_file
        int 0x97
        mov si, font_file
        int 0x91
        call display_update_font
        call crlf

        ; Now that we have a font file, switch to gfx mode
        mov al, VIDEOMODE_GFX_320_200_COLOR ; gfx mode
        int 0x93 ; Switch video mode

                            ; -------------------Testcode
                            ;int 0xf1 ; GFX Test

                            ; Wait 2 seconds
                            ;mov ax, 2000
                            ;int 0x81

                            ;int 0x95 ; cls

        jmp .after_gfx_mode
    .no_font:
        push si
            mov al, 2
            mov si, .error_no_font_file
            int 0x97
        pop si

        int 0x91
        call crlf

        jmp .after_gfx_mode
    .skip_gfx_mode:
        push si
            mov al, 1
            mov si, .warn_no_gfx_mode
            int 0x97
        pop si

        jmp .after_gfx_mode
    .info_reading_font_file db "Reading font file: ", 0
    .error_no_font_file db "Font file does not exist! Graphics mode disabled! : ", 0
    .warn_no_gfx_mode db "GFX mode not enabled!", ASCII_CR, ASCII_LF, 0
    .after_gfx_mode:

    ; check if shell exists
    mov si, shell_file
    int 0xc1
    cmp ax, -1
    jz .no_shell
        ; read shell
        mov al, 0
        mov si, .info_reading_shell_file
        int 0x97
        mov si, shell_file
        int 0x91

        ; check cmd line
        push cs
        pop es
        mov di, .empty_shell_cmd
        cmp word [cs:shell_cmd_segment], 0
        jz .after_shell_cmd
            call crlf

            push word [cs:shell_cmd_segment]
            pop es
            mov di, [cs:shell_cmd_offset]

            push ds
            push si
                mov al, 0
                mov si, .info_shell_param
                int 0x97 ; print prefix

                push es
                pop ds
                mov si, di

                int 0x91 ; println
            pop si
            pop ds
        .after_shell_cmd:

        mov byte [cs:fat_print_dot_on_disk_access], FALSE ; no feedback for disk operation for shell
        int 0xf2 ; call shell
        mov byte [cs:fat_print_dot_on_disk_access], TRUE 

        jmp .after_shell
    .no_shell:
        mov al, 2
        mov si, .error_no_shell_file
        int 0x97

        ; Wait 5 seconds befor starting the mlt
        mov ax, 5000
        int 0x81

        jmp .after_shell
    .info_reading_shell_file db "Loading shell: ", 0
    .info_shell_param db "Using shell parameters: ", 0
    .error_no_shell_file db "Shell does not exist!", ASCII_CR, ASCII_LF, 0
    .empty_shell_cmd db 0
    .after_shell:
    
    call test_code

    ; set default video mode and start mlt
    call display_restore_default_kernel_video_mode
    int 0x95 ; cls

    int 0xf0 ; mlt

    cli
    jmp 0xffff:0 ; reset vector

    test_code:
        push cs
        pop ds
        mov si, test_file
        ;int 0xc2

        ret

    test_file db "TESTFILE.TST", 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Prints a crlf
;-------------------------------------------------------------------------------
crlf:
    mov al, ASCII_CR
    int 0x90
    mov al, ASCII_LF
    int 0x90

    ret
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

        push dx
        push bp
            mov al, 0x08 ; pit timer
            mov bx, timer_callback
            call kernel_install_interrupt
        pop bp
        pop dx

        call timer_init
    pop ds

    mov al, 0x80
    mov bx, timer_get_ticks
    call kernel_install_interrupt

    mov al, 0x81
    mov bx, timer_sleep_millis
    call kernel_install_interrupt

    mov al, 0x82
    mov bx, timer_sleep_seconds
    call kernel_install_interrupt

    ret

install_math:
    mov al, 0x8e
    mov bx, math_32_bit_operation
    call kernel_install_interrupt

    ret

install_sound:
    mov al, 0x88
    mov bx, snd_output
    call kernel_install_interrupt

    mov al, 0x89
    mov bx, snd_stop_output
    call kernel_install_interrupt

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

    mov al, 0x96
    mov bx, display_get_video_mode
    call kernel_install_interrupt

    mov al, 0x97
    mov bx, display_prefix_print
    call kernel_install_interrupt

    mov al, 0x98
    mov bx, display_get_screen_pos
    call kernel_install_interrupt

    mov al, 0x99
    mov bx, display_set_screen_pos
    call kernel_install_interrupt

    mov al, 0x9a
    mov bx, display_set_color
    call kernel_install_interrupt

    mov al, 0x9b
    mov bx, display_double_buffer_ctrl
    call kernel_install_interrupt

    mov si, .wait_msg
    int 0x91

    ret

    .wait_msg db "WAIT", ASCII_CR, ASCII_LF, 0

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

    mov al, 0xb3
    mov bx, keyboard_event_registrator
    call kernel_install_interrupt

    mov al, 0xb4
    mov bx, keyboard_ascii_to_scancode
    call kernel_install_interrupt

    mov al, 0xb5
    mov bx, keyboard_clear_buffer
    call kernel_install_interrupt

    call keyboard_init

    ret

install_memory:
    call memory_initialize_all

    mov al, 0xb8
    mov bx, memory_allocate
    call kernel_install_interrupt

    mov al, 0xb9
    mov bx, memory_free
    call kernel_install_interrupt

    ret

install_fat:
    mov al, 0xc0
    mov bx, fat_load
    call kernel_install_interrupt

    mov al, 0xc1
    mov bx, fat_get_file_size
    call kernel_install_interrupt

    mov al, 0xc2
    mov bx, fat_open
    call kernel_install_interrupt

    mov al, 0xc3
    mov bx, fat_close
    call kernel_install_interrupt

    mov al, 0xc4
    mov bx, fat_read_next_byte
    call kernel_install_interrupt

    mov al, 0xc5
    mov bx, fat_read_next_string
    call kernel_install_interrupt

    mov al, 0xc6
    mov bx, fat_write_next_byte
    call kernel_install_interrupt

    mov al, 0xc7
    mov bx, fat_write_next_bytes
    call kernel_install_interrupt

    mov al, 0xc8
    mov bx, fat_directory_exists
    call kernel_install_interrupt

    mov al, 0xc9
    mov bx, fat_directory_load
    call kernel_install_interrupt

    mov al, 0xca
    mov bx, fat_get_current_dir
    call kernel_install_interrupt

    mov al, 0xcf
    mov bx, fat_callback_disk_operation
    call kernel_install_interrupt

    mov dl, [cs:bootdrive]
    mov [cs:fat_current_drive], dl 

    ret

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

    mov al, 0xd5
    mov bx, str_remove_whitespace
    call kernel_install_interrupt

    mov al, 0xd6
    mov bx, str_replace_char
    call kernel_install_interrupt

    mov al, 0xd7
    mov bx, str_convert_case
    call kernel_install_interrupt

    ret

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

    mov al, 0xe3
    mov bx, convert_unsigned_16bit_atoi
    call kernel_install_interrupt

    mov al, 0xe4
    mov bx, convert_bin_ascii_hex
    call kernel_install_interrupt

    ret

install_misc:
    mov al, 0x20
    mov bx, dos_compatibility_terminate
    call kernel_install_interrupt

    mov al, 0x21
    mov bx, dos_compatibility
    call kernel_install_interrupt

    mov al, 0xf0
    mov bx, mlt_start
    call kernel_install_interrupt

    mov al, 0xf2
    mov bx, misc_start_com
    call kernel_install_interrupt

    mov al, 0xf4
    mov bx, timer_create_random_number
    call kernel_install_interrupt

    ret
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
; Enables an hardware interrupt via the PIC
;
; input: 
;   BL -> interrupt line (0-15) -> 2: disable second PIC
;
; output:
;   none
;-------------------------------------------------------------------------------
kernel_enable_hardware_interrupt:
    push ax
    push bx
    push cx
    push dx

    cmp bl, 15 ; There are no more than 15 hardware interrupts
    jg .done

    cmp bl, 8 ; 0 - 7 -> PIC 1, 8 - 15 -> PIC 2
    jge .second_pic
        mov dx, PIC1_DATA
        jmp .unmask

    .second_pic:
        mov dx, PIC2_DATA
        sub bl, 8 ; Calculate interrupt as seen by PIC 2

    .unmask:
    in al, dx ; Get current mask

    ; Calculate new mask https://wiki.osdev.org/8259_PIC#Masking
    mov bh, 1 
    mov cl, bl
    shl bh, cl
    not bh
    and al, bh

    out dx, al ; Write new mask

    .done:
        pop ax
        pop bx
        pop cx
        pop dx
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Disables an hardware interrupt via the PIC
;
; input: 
;   BL -> interrupt line (0-15) -> 2: disable second PIC
;
; output:
;   none
;-------------------------------------------------------------------------------
kernel_disable_hardware_interrupt:
    push ax
    push bx
    push cx
    push dx

    cmp bl, 15 ; There are no more than 15 hardware interrupts
    jg .done

    cmp bl, 8 ; 0 - 7 -> PIC 1, 8 - 15 -> PIC 2
    jge .second_pic
        mov dx, PIC1_DATA
        jmp .mask

    .second_pic:
        mov dx, PIC2_DATA
        sub bl, 8 ; Calculate interrupt as seen by PIC 2

    .mask:
    in al, dx ; Get current mask

    ; Calculate new mask https://wiki.osdev.org/8259_PIC#Masking
    mov bh, 1
    mov cl, bl
    shl bh, cl
    or al, bh
    
    out dx, al ; Write new mask

    .done:
        pop ax
        pop bx
        pop cx
        pop dx
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
    mov al, PIC_EOI_MASK ; send end of interrupt (eoi)
    out PIC1_COMMAND, al ; send eoi to master pic

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
    mov al, PIC_EOI_MASK ; send end of interrupt (eoi)
    out PIC1_COMMAND, al ; send eoi to master pic
    out PIC2_COMMAND, al ; send eoi to slave pic

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Read kernel config file and process it
;
; input:
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
process_config_file:
    push ds
    push es

    ; Create string for the ident
    mov ax, .MAX_STR_LENGTH ; max length
    int 0xb8
    mov [cs:.str_segment], bx
    mov [cs:.str_offset], dx

    ; Create string for the parameter
    mov ax, .MAX_STR_LENGTH ; max length
    int 0xb8
    mov [cs:.parameter_segment], bx
    mov [cs:.parameter_offset], dx

    push cs
    pop ds

    mov al, 0
    mov si, .info_reading_config
    int 0x97

    mov si, kernel_config_file
    int 0xc2 ; Open file
    mov [cs:.file_segment], bp
    mov [cs:.file_offset], bx
    call crlf

    .loop:
        cmp byte [cs:.process_file], FALSE
        jz .exit_loop

        ; Read string from file into string 
        mov bp, [cs:.file_segment]
        mov bx, [cs:.file_offset]
        push word [cs:.str_segment]
        pop es
        mov di, [cs:.str_offset]
        mov ah, 1 ; read string to line break
        mov cx, .MAX_STR_LENGTH
        int 0xc5

        cmp dh, 0x01 ; If not eof, skip setting process_file to false
        jnz .skip_set_exit_loop
            mov byte [cs:.process_file], FALSE
        .skip_set_exit_loop:

        cmp dh, 0 ; Check if an error occured
        jnz .skip_entry

        ; remove comment
        push es
            push cs
            pop es
            push word [cs:.str_segment]
            pop ds
            mov si, [cs:.str_offset]
            mov di, .process_config_file_dummy
            mov ah, 2
            mov al, byte [cs:.process_config_file_comment]
            mov cx, 1
            int 0xd4 ; Split string
        pop es

        ; remove all whitespace from string
        push ax
            mov ah, 1
            int 0xd5
        pop ax

        ; check if string is empty
        cmp byte [ds:si], 0
        jz .loop ; next entry

        ; Display configuration directive
        call .info_current_directive

        ; Split string on = sign
        mov ah, 0b10 ; termination of the first and second string
        mov al, ASCII_EQUALS
        mov cx, .MAX_STR_LENGTH
        ; ds:si is already set
        push word [cs:.parameter_segment]
        pop es
        mov di, [cs:.parameter_offset]
        int 0xd4

        ; Check for ident and set vars accordingly
        ; Keyboard map
        mov ah, 11b ; Check if equal, caseinsensitive
        ; ds:si already set
        push cs
        pop es
        mov di, .config_kbrd_ident
        int 0xd2
        jnc .set_keyboard_map ; set var, if equal

        ; Textmode color
        mov ah, 11b ; Check if equal, caseinsensitive
        ; ds:si already set
        push cs
        pop es
        mov di, .config_txt_color_ident
        int 0xd2
        jnc .set_txt_color ; set var, if equal

        ; Shell file
        mov ah, 11b ; Check if equal, caseinsensitive
        ; ds:si already set
        push cs
        pop es
        mov di, .config_shell_ident
        int 0xd2
        jnc .set_shell_file ; set var, if equal

        ; Shell parameter
        mov ah, 11b ; Check if equal, caseinsensitive
        ; ds:si already set
        push cs
        pop es
        mov di, .config_shell_cmd_ident
        int 0xd2
        jnc .set_shell_params ; set var, if equal

        ; Font file
        mov ah, 11b ; Check if equal, caseinsensitive
        ; ds:si already set
        push cs
        pop es
        mov di, .config_font_ident
        int 0xd2
        jnc .set_font_file ; set var, if equal

        ; gfx mode color
        mov ah, 11b ; Check if equal, caseinsensitive
        ; ds:si already set
        push cs
        pop es
        mov di, .config_gfx_color_ident
        int 0xd2
        jnc .set_gfx_color ; set var, if equal

        jmp .error_no_directive

        .skip_entry: 
            jmp .loop
    .exit_loop:

    mov bp, [cs:.file_segment]
    mov bx, [cs:.file_offset]
    int 0xc3 ; Close file

    .done:
        ; Deallocate strings
        mov ax, .MAX_STR_LENGTH ; max length
        mov bx, [cs:.str_segment]
        mov dx, [cs:.str_offset]
        int 0xb9

        mov ax, .MAX_STR_LENGTH ; max length
        mov bx, [cs:.parameter_segment]
        mov dx, [cs:.parameter_offset]
        int 0xb9

        pop es
        pop ds

        ret

    .set_keyboard_map:
        push word [cs:.parameter_segment]
        pop ds
        mov si, [cs:.parameter_offset]

        ; Check if file exists
        int 0xc1
        cmp ax, -1
        jz .error_file_not_found

        push cs
        pop es
        mov di, keymap_file
        mov bx, 0 ; Start from begin of string
        mov dx, FAT_INPUT_LENGTH ; Max file length
        int 0xd3 ; Copy substring to string

        jmp .loop

    .set_txt_color:
        push word [cs:.parameter_segment]
        pop ds
        mov si, [cs:.parameter_offset]

        int 0xe3
        jc .error_parameter ; skip if error

        ; Check if number greater than 0xff
        cmp ax, 0xff
        ja .error_parameter

        ; Check if nibbles are equal
        mov bh, al
        mov bl, al
        shr bh, 4
        and bl, 0x0f
        cmp bh, bl
        jz .error_parameter

        ; Write color code to display vars
        ; Do explicitly NOT use the interrupt, because it requires the video mode to be updated -> May be adjusted by other settings in the config file
        ;int 0x9a
        mov [cs:display_color], al

        ; Update info/warning/error/critical backgrounds
        and al, 0xf0 ; delete foreground information

        and byte [cs:display_info_color], 0x0f ; delete background information
        or byte [cs:display_info_color], al

        and byte [cs:display_warning_color], 0x0f ; delete background information
        or byte [cs:display_warning_color], al

        and byte [cs:display_error_color], 0x0f ; delete background information
        or byte [cs:display_error_color], al

        and byte [cs:display_critical_color], 0x0f ; delete background information
        or byte [cs:display_critical_color], al

        jmp .loop

    .set_shell_file:
        push word [cs:.parameter_segment]
        pop ds
        mov si, [cs:.parameter_offset]

        ; Check if file exists
        int 0xc1
        cmp ax, -1
        jz .error_file_not_found

        push cs
        pop es
        mov di, shell_file
        mov bx, 0 ; Start from begin of string
        mov dx, FAT_INPUT_LENGTH ; Max file length
        int 0xd3 ; Copy substring to string

        jmp .loop

    .set_shell_params:
        push word [cs:.parameter_segment]
        pop ds
        mov si, [cs:.parameter_offset]

        int 0xd0 ; get string length excluding terminator
        inc ax

        int 0xb8 ; reserve memory for command line arguments

        ; Save location of string
        mov [cs:shell_cmd_segment], bx
        mov [cs:shell_cmd_offset], dx

        mov es, bx
        mov di, dx
        mov bx, 0 ; Start from begin of string
        mov dx, ax ; Length of string
        int 0xd3 ; Copy substring to string

        ; Replace comma with space
        push ds
        push si
            push es
            pop ds
            mov si, di

            mov cx, 0xffff ; replace any occurence
            mov dl, ASCII_COMMA
            mov dh, ASCII_SP
            int 0xd6 ; replace char
        pop si
        pop ds

        jmp .loop

    .set_font_file:
        push word [cs:.parameter_segment]
        pop ds
        mov si, [cs:.parameter_offset]

        ; Check if file exists
        int 0xc1
        cmp ax, -1
        jz .error_file_not_found

        push cs
        pop es
        mov di, font_file
        mov bx, 0 ; Start from begin of string
        mov dx, FAT_INPUT_LENGTH ; Max file length
        int 0xd3 ; Copy substring to string

        jmp .loop 

    .set_gfx_color:
        push word [cs:.parameter_segment]
        pop ds
        mov si, [cs:.parameter_offset]

        int 0xe3
        jc .error_parameter ; skip if error

        ; Check if bytes are equal
        cmp ah, al
        jz .error_parameter

        ; Write color code to display vars
        ; Do explicitly NOT use the interrupt, because it requires the video mode to be updated -> May be adjusted by other settings in the config file
        ; int 0x9a
        mov [cs:display_gfx_bg_color], ah
        mov [cs:display_gfx_fg_color], al

        jmp .loop

    ; Print an info with the current directive in ds:si
    .info_current_directive:
        mov al, 0 ; Info
        int 0x97

        call crlf

        ret

    ; Print a warning text, that the current config directrive is not recognized
    .error_no_directive:
        push ds
        push si
            push cs
            pop ds
            mov si, .warn_directive_not_recognized
            mov al, 1 ; Warning
            int 0x97
        pop si
        pop ds

        jmp .loop ; next entry

    ; Print a warning text, that the current config directrive contains errors
    .error_parameter:
        push ds
        push si
            push cs
            pop ds
            mov si, .warn_directive_contains_errors
            mov al, 1 ; Warning
            int 0x97
        pop si
        pop ds

        jmp .loop ; next entry

    ; Print a warning text, that the current file does not exist
    .error_file_not_found:
        ; ds:si locates to string of current file
        mov al, 1 ; Warning
        int 0x97

        push ds
        push si
            push cs
            pop ds
            mov si, .warn_not_found_default
            int 0x91
        pop si
        pop ds

        jmp .loop

    .config_kbrd_ident db "KBRD_MAP", 0
    .config_txt_color_ident db "TXT_COLOR", 0
    .config_shell_ident db "SHELL", 0
    .config_shell_cmd_ident db "SHELL_PARAMS", 0
    .config_font_ident db "FONT", 0
    .config_gfx_color_ident db "GFX_COLOR", 0
    .warn_not_found_default db " not found! Using default instead.", ASCII_CR, ASCII_LF, 0
    .warn_directive_contains_errors db "Configuration directive caused an error!", ASCII_CR, ASCII_LF, 0
    .warn_directive_not_recognized db "Configuration directive not recognized!", ASCII_CR, ASCII_LF, 0
    .info_reading_config db "Reading kernel configuration file KERNEL.CFG", 0
    .file_segment dw 0
    .file_offset dw 0
    .str_segment dw 0
    .str_offset dw 0
    .parameter_segment dw 0
    .parameter_offset dw 0
    .process_file db TRUE
    .MAX_STR_LENGTH equ 128
    .process_config_file_comment db ";"
    .process_config_file_dummy dw 0 ; one byte for dummy string, one byte terminator
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Speaks the ultimate truth, always
;-------------------------------------------------------------------------------
oracle:
    pusha
    push ds

    push cs
    pop ds

    int 0xf4
    test al, 1
    jnz .yes
    .no:
        mov si, .no_str
        jmp .print
    .yes:
        mov si, .yes_str

    .print:
    int 0x91

    pop ds
    popa
    retf

    .yes_str db "YES", 0x0a, 0x0d, 0
    .no_str db "NO", 0x0a, 0x0d, 0
;-------------------------------------------------------------------------------

change_sys_drive_test:
    xchg bx, bx
    mov byte [cs:fat_current_drive], 80h
    mov byte [cs:bsDriveNumber], 80h
    call fat_load_root_directory
    mov al, ASCII_1
    int 0x90
    retf


;-------------------------------------------------------------------------------
; Draw rectangles on the screen to test colors
;-------------------------------------------------------------------------------
_kernel_test_gfx:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    mov cx, 8
    mov dl, 0
    .draw_rectangles:
        mov al, cl
        mov dh, 10
        mul dh
        push cx
        mov cx, 32
        .draw_rectangle:
            mov bx, cx
            push dx
                mov dx, 10
                xchg ax, bx
                mul dx
                xchg ax, bx
            pop dx
            push ax
            push cx
            mov cx, 10
            .put_line:
                push cx
                push bx
                mov cx, 10
                .put_pixel:
                    int 0x94
                    dec bx
                loop .put_pixel
                pop bx
                pop cx
                dec al
            loop .put_line
            pop cx
            pop ax
            inc dl
        loop .draw_rectangle
        pop cx
    loop .draw_rectangles
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Includes
;-------------------------------------------------------------------------------
%include "KRNLTMER.INC"
%include "KRNLSND.INC"
%include "KRNLMATH.INC"
%include "KRNLDISP.INC"
%include "KRNLKBRD.INC"
%include "KRNLMEM.INC"
%include "KRNLFAT.INC"
%include "KRNLSTR.INC"
%include "KRNLCVRT.INC"
%include "KRNLMISC.INC"
%include "KRNLDOS.INC"
%include "KRNLCSIC.INC"
;-------------------------------------------------------------------------------