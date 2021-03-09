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
%include "ASCII.INC"
%include "COM_HEAD.INC"
jmp MAIN
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
INPUT_BUFFER_LENGTH     equ 80

FAT_RD_ENTRY_LENGTH         equ 32
FAT_RD_NAME_OFFSET          equ 0x00
FAT_RD_EXT_OFFSET           equ 0x08
FAT_RD_ATTRIB_OFFSET        equ 0x0b
    FAT_RD_ATTRIB_READ_ONLY_OFFSET      equ 0x01
    FAT_RD_ATTRIB_HIDDEN_OFFSET         equ 0x02
    FAT_RD_ATTRIB_SYSTEM_OFFSET         equ 0x04
    FAT_RD_ATTRIB_VOLUME_LBL_OFFSET     equ 0x08
    FAT_RD_ATTRIB_SUB_DIR_OFFSET        equ 0x10
    FAT_RD_ATTRIB_ARCHIVE_OFFSET        equ 0x20
    FAT_RD_ATTRIB_DEVICE_OFFSET         equ 0x40
    FAT_RD_ATTRIB_UNUSED                equ 0x80
FAT_RD_CLUSTER_OFFSET       equ 0x1a
FAT_RD_SIZE_OFFSET          equ 0x1c

MAX_ROOT_ENTRIES            equ 224

DIR_SIZE_OFFSET             equ 19
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Vars
;-------------------------------------------------------------------------------
welcome_msg             db "TONY shell program loaded.", ASCII_LF, ASCII_LF, 0
current_directory       times 45 db 0
                        db 0
ready_indicator         db ">", 0

input_buffer            times INPUT_BUFFER_LENGTH db 0
split_buffer            times INPUT_BUFFER_LENGTH db 0

cmd_file_not_found      db "Command or file not found!", ASCII_LF, 0

com_ident               db ".COM", 0

run                     db TRUE

enable_hacking_game_ident db "-fallout_style"
enable_hacking_game       db FALSE

fo_mode_match_ident      db "-match", 0
fo_mode_keystroke_ident  db "-keystroke", 0
fo_mode_auto_ident       db "-auto", 0
fo_mode                  db 0 ; (0 = match, 1 = keystroke, 2 = auto)

fo_level_ident           db "-lv=", 0
fo_pw_level              db 0 ; 0 - 4 ; 5 characters, 8 characters, 10 characters, 12 characters, 15 characters

fo_sound_ident           db "-snd", 0
fo_use_sound             db FALSE

fo_fast_mode_ident       db "-fast", 0
fo_fast_mode             db FALSE

cmd_dir_ident            db "dir", 0
cmd_cd_ident             db "cd", 0
cmd_cls_ident            db "cls", 0
cmd_color_ident          db "color", 0
cmd_exit_ident           db "exit", 0
cmd_path_ident           db "path", 0
cmd_set_path_ident       db "set path", 0
cmd_halt_ident           db "halt", 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Program entry point
;-------------------------------------------------------------------------------
MAIN:
    xor cx, cx
    mov cl, [cs:argc]
    cmp cl, 0
    jz .no_params
        .extract_params:
            mov si, argv
            mov di, enable_hacking_game_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_hacking_game
                mov byte [cs:enable_hacking_game], TRUE
            .no_hacking_game:

            mov di, fo_mode_match_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_mode_match
                mov byte [cs:fo_mode], 0
            .no_mode_match:

            mov di, fo_mode_keystroke_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_mode_keystroke
                mov byte [cs:fo_mode], 1
            .no_mode_keystroke:

            mov di, fo_mode_auto_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_mode_auto
                mov byte [cs:fo_mode], 2
            .no_mode_auto:

            mov di, fo_level_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .no_level
                ; ax contains relative start address of "-lv="
                push si
                    add si, ax
                    add si, 4 ; characters for -lv= -> si now points to the level
                    lodsb
                pop si

                sub al, ASCII_0 ; ascii adjust
                cmp al, 4
                ja .no_level ; al is not between 0 and 4 -> use default values
                    mov [ds:fo_pw_level], al
            .no_level:

            mov di, fo_sound_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .fo_no_sound
                mov byte [cs:fo_use_sound], TRUE
            .fo_no_sound:

            mov di, fo_fast_mode_ident
            mov ah, 0b10 ; contains substring, ignore case
            int 0xd2
            jc .fo_no_fast_mode
                mov byte [cs:fo_fast_mode], TRUE
            .fo_no_fast_mode:
    .no_params:

    mov al, TRUE
    cmp byte [ds:enable_hacking_game], al
    jnz .skip_hacking_game
        call fallout_hacking_game
    .skip_hacking_game:

    mov si, welcome_msg
    int 0x91 ; print

    jmp shell_loop
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
shell_loop:
    mov si, current_directory
    int 0x91
    mov si, ready_indicator
    int 0x91

    mov al, 0
    mov cx, INPUT_BUFFER_LENGTH
    mov di, split_buffer
    int 0xd1 ; intiailze string
    mov di, input_buffer
    int 0xd1 ; intiailze string
    int 0xb1 ; readln

    cmp byte [es:di], 0
    jz shell_loop

    mov si, input_buffer
    mov di, split_buffer
    mov ah, 0b10 ; terminate first and second string
    mov al, ASCII_SP
    mov cx, INPUT_BUFFER_LENGTH
    int 0xd4 ; split string

    call try_exec_file
    jnc .check_run

    call try_exec_cmd
    jnc .check_run

    mov al, 2 ; error prefix
    mov si, cmd_file_not_found
    int 0x97 ; prefix print

    .check_run:
        cmp byte [cs:run], TRUE
        jz shell_loop

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Searches for a executable file with its name in input_buffer
; Executes it, if it exists, giving it arguments in split_buffer
; input:
;   input_buffer -> Filename
;   split_buffer -> File arguments
; ouput:
;   carry flag
;       -> set, if file does not exist
;       -> clear, if file executed
;-------------------------------------------------------------------------------
try_exec_file:
    ; check if file is executable
    mov si, input_buffer
    mov ah, 0b10 ; contains substring, ignore case
    mov di, com_ident
    int 0xd2 ; check if string contains substring
    jc .file_non_existant

    ; si is set to input buffer
    mov di, split_buffer
    int 0xf2 ; call com program
    cmp ax, 0x01 ; file not found error
    jz .file_non_existant
    clc
    jmp .done

    .file_non_existant:
        stc
    .done:
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Searches for the command in input_buffer and executes it giving the parameters
; in split_buffer
; input:
;   input_buffer -> Commandname
;   split_buffer -> Command arguments
; ouput:
;   carry flag
;       -> set, if command does not exist
;       -> clear, if command executed
;-------------------------------------------------------------------------------
try_exec_cmd:
    push word .cmd_return_point ; commands exit via ret instruction
                                ; they should all return to this central point

    ; check commands
    mov si, input_buffer
    mov ah, 0b11 ; ignore case, string equals

    mov di, cmd_dir_ident
    int 0xd2 ; check if string contains substring
    jnc dir

    mov di, cmd_cd_ident
    int 0xd2 ; check if string contains substring
    jnc cd

    mov di, cmd_cls_ident
    int 0xd2 ; check if string contains substring
    jnc cls

    mov di, cmd_color_ident
    int 0xd2 ; check if string contains substring
    jnc color

    mov di, cmd_exit_ident
    int 0xd2 ; check if string contains substring
    jnc exit

    mov di, cmd_path_ident
    int 0xd2 ; check if string contains substring
    jnc path

    mov di, cmd_set_path_ident
    int 0xd2 ; check if string contains substring
    jnc set_path

    mov di, cmd_halt_ident
    int 0xd2 ; check if string contains substring
    jnc halt

    jmp .cmd_non_existant

    .cmd_return_point:
    clc
    jmp .done

    .cmd_non_existant:
        pop ax ; command not called, remove return address from stack
        stc
    .done:
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Displays the contents of the current directory
;-------------------------------------------------------------------------------
dir:
    push ds
        push word ROOT_SEGMENT
        pop ds

        mov byte [cs:.file_flags], 0
        
        ; Display system volume information
        mov si, ROOT_OFFSET
        mov al, FAT_RD_ATTRIB_ARCHIVE_OFFSET + FAT_RD_ATTRIB_VOLUME_LBL_OFFSET
        mov cx, MAX_ROOT_ENTRIES
        .serach_volume_info:
            cmp byte [ds:si], 0
            jz .volume_label_done ; no more entries

            cmp [ds:si + FAT_RD_ATTRIB_OFFSET], al
            jz .display_volume_info

            add si, FAT_RD_ENTRY_LENGTH
        loop .serach_volume_info
        .display_volume_info:
            push ds
            push si
                push cs
                pop ds
                mov si, .volume_label
                int 0x91 ; print
            pop si
            pop ds

            mov cx, FAT_RD_ATTRIB_OFFSET ; length of volume label
            .putch_volume_info:
                lodsb
                int 0x90 ; putch
            loop .putch_volume_info

            mov al, ASCII_LF
            int 0x90 ; linebreak
        .volume_label_done:


        ; Display any subdirectory (if any)
        mov si, ROOT_OFFSET
        mov cx, MAX_ROOT_ENTRIES
        .serach_directories:
            cmp byte [ds:si], 0
            jz .directories_done ; no more entries

            mov al, FAT_RD_ATTRIB_SUB_DIR_OFFSET
            or al, [ds:.file_flags]
            cmp [ds:si + FAT_RD_ATTRIB_OFFSET], al
            jnz .next_directory

            ; display directory
            push cx
                mov cx, FAT_RD_ATTRIB_OFFSET ; length of directory name
                .putch_directory_name:
                    lodsb
                    int 0x90 ; putch
                loop .putch_directory_name
            pop cx

            push ds
            push si
                push cs
                pop ds
                mov si, .dir_label
                int 0x91 ; print
            pop si
            pop ds

            mov al, ASCII_LF
            int 0x90 ; linebreak

            .next_directory:
                add si, FAT_RD_ENTRY_LENGTH
        loop .serach_directories
        .directories_done:

        ; Display any file matching the file flag (if any) with its size
        mov si, ROOT_OFFSET
        mov cx, MAX_ROOT_ENTRIES
        .serach_files:
            cmp byte [ds:si], 0
            jz .files_done ; no more entries

            mov bp, si

            mov al, [cs:.file_flags]
            cmp [ds:si + FAT_RD_ATTRIB_OFFSET], al
            jnz .next_file

            ; display filename
            push cx
                mov cx, FAT_RD_EXT_OFFSET ; length of file name
                .putch_file_name:
                    lodsb
                    int 0x90 ; putch
                loop .putch_file_name
            pop cx

            mov al, ASCII_SP
            int 0x90 ; putch

            ; display filextension
            push cx
                mov cx, FAT_RD_ATTRIB_OFFSET - FAT_RD_EXT_OFFSET ; length of file name
                .putch_file_ext:
                    lodsb
                    int 0x90 ; putch
                loop .putch_file_ext
            pop cx

            ; Display size (build size from least significant digit to most significant digit)
            call .fill_size_str

            mov ah, 1
            int 0x98 ; get current position
            mov bx, DIR_SIZE_OFFSET
            int 0x99 ; set current position

            push ds
            push si
                push cs
                pop ds
                mov si, .size_str
                int 0x91 ; print
            pop si
            pop ds

            mov al, ASCII_LF
            int 0x90 ; linebreak

            .next_file:
                mov si, bp
                add si, FAT_RD_ENTRY_LENGTH
        loop .serach_files
        .files_done:

    pop ds
    ret

    .file_flags     db 0
    .volume_label   db 'Volume label: ', 0
    .dir_label      db ' <DIR> ', 0
    .size_str       times 8 db 0
                    db 0 ; zero termination for size str

    ; Fills the size_str with the size of the currently addressed file via ds:bp
    ; File size is filled with leading spaces
    .fill_size_str:
        pusha
        push es
            push cs
            pop es

            mov di, .size_str

            mov byte [cs:.display_zeros], FALSE ; do not display leading zeros
            add bp, FAT_RD_SIZE_OFFSET + 3 ; offset to most significant byte

            mov cx, 4

            .fill_str:
                mov dl, [ds:bp]
                int 0xe4 ; transform to printable hex format

                cmp byte [cs:.display_zeros], FALSE
                jnz .write_to_var
                    ; If entries are zero, write space instead
                    cmp dh, ASCII_0
                    jnz .write_to_var
                    mov dh, ASCII_SP

                    cmp dl, ASCII_0
                    jnz .write_to_var
                    mov dl, ASCII_SP
                .write_to_var:
                mov al, dh
                stosb
                mov al, dl
                stosb
                
                dec bp
                cmp dx, ASCII_SP * 256 + ASCII_SP
                jz .next
                    mov byte [cs:.display_zeros], TRUE ; there was at least one character printed, spaces can no longer be omitted!
                .next:
            loop .fill_str
        pop es
        popa
        ret

        .display_zeros db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
cd:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Clears the screen
;-------------------------------------------------------------------------------
cls:
    int 0x95
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
color:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Shell returns control to caller after command is interpreted
;-------------------------------------------------------------------------------
exit:
    mov byte [cs:run], FALSE
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
path:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
set_path:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Halts the machine
;-------------------------------------------------------------------------------
halt:
    cli
    hlt
    ret ; test if halt behaves like intended on real hardware
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%include "SHELL_FO.INC"
;-------------------------------------------------------------------------------
