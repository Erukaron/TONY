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
%include "SCANCODE.INC"
jmp MAIN
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
BUFFER_SIZE     equ 0xffff
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
mtx_col                 db 0
mtx_line                db 0

page_cols               db 0 ; depends on video mode -> 80*33 (gfx) or 80*25 (text)
page_lines              db 0 

buffer_segment          dw 0
buffer_offset           dw 0

buffer_current_pos      dw 0 ; absolute offset from di = 0
buffer_page_offset      dw 0 ; offset for currently displayed page from di = 0
buffer_last_pos         dw 0 ; last byte containing a value

stack_start             dw 0

escape_handler_segment  dw 0
escape_handler_offset   dw 0
left_handler_segment    dw 0
left_handler_offset     dw 0
right_handler_segment   dw 0
right_handler_offset    dw 0
down_handler_segment    dw 0
down_handler_offset     dw 0
up_handler_segment      dw 0
up_handler_offset       dw 0

init_finished           db FALSE
video_mode              db 0
cursor_symbol           db '_'
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Entrypoint, (de)allocation of buffer
;-------------------------------------------------------------------------------
MAIN:
    ; intialize vars
    cli
    mov [cs:stack_start], sp
    sti
    mov byte [cs:init_finished], FALSE
    mov byte [cs:mtx_col], 0
    mov byte [cs:mtx_line], 0
    mov word [cs:buffer_current_pos], 0
    mov word [cs:buffer_page_offset], 0
    mov word [cs:buffer_last_pos], 0

    mov byte [cs:page_cols], 80 ; default is text mode
    mov byte [cs:page_lines], 24 
    int 0x96 ; get current video mode
    mov [cs:video_mode], al
    test al, VIDEO_MODE_GFX_320x200
    jz .skip_video_mode
        mov byte [cs:page_lines], 32 ; in case of video mode, adjust page size
    .skip_video_mode:

    mov ax, BUFFER_SIZE
    int 0xb8 ; Allocate memory for buffer
    cmp dx, 0xffff
    jz ctrl_to_caller

    mov [cs:buffer_segment], bx
    mov [cs:buffer_offset], dx

    ; set write buffer to allocated memory
    ; Offset needs to be zero based
    shr dx, 4
    add bx, dx
    xor dx, dx
    mov es, bx
    mov di, dx

    ; Register keyboard events
    call register_events

    ; Load/create file (param)
    cmp byte [cs:argc], 0
    jz .skip_load_file
        mov si, argv
        call load_file_si_to_buffer
    .skip_load_file:

    mov byte [cs:init_finished], TRUE
    call program_loop

deallocate_memory:
    call deregister_events

    mov ax, BUFFER_SIZE
    mov bx, [cs:buffer_segment]
    mov dx, [cs:buffer_offset]
    int 0xb9 ; deallocate memory

ctrl_to_caller:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Loads (or creates) the file specified by ds:si into the buffer at es:di
;-------------------------------------------------------------------------------
load_file_si_to_buffer:
    int 0xc2 ; open or create file

    .next_byte:
        int 0xc4 ; read byte

        cmp dh, 1
        jz .done ; eof

        cmp dh, 0
        jnz .error

        mov [es:di], dl
        inc di
        inc word [cs:buffer_last_pos]
    jmp .next_byte

    .error:
        xchg bx, bx
        xor ax, ax ; default
        int 0x88 ; Output sound
        mov ax, 500
        int 0x81 ; sleep
        int 0x89 ; stop sound

    ;.done: ; test
        xor ax, ax
        mov si, ax
        push es
        pop ds
        .disp:
            int 0x91 
            mov ax, 300
            int 0x81
        jmp .disp

    .done:
        int 0xc3 ; close file

        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
register_events:
    ; Escape
    mov ah, 0
    mov al, 0x80 ; on key release
    or al, SCAN_CODE_ESCAPE
    mov bx, cs
    mov dx, handle_escape
    int 0xb3
    mov [cs:escape_handler_segment], bx
    mov [cs:escape_handler_offset], bx

    ; Left
    mov ah, 0
    mov al, SCAN_CODE_LEFT_ARROW
    mov bx, cs
    mov dx, handle_left
    int 0xb3
    mov [cs:left_handler_segment], bx
    mov [cs:left_handler_offset], bx

    ; Right
    mov ah, 0
    mov al, SCAN_CODE_RIGHT_ARROW
    mov bx, cs
    mov dx, handle_right
    int 0xb3
    mov [cs:right_handler_segment], bx
    mov [cs:right_handler_offset], bx

    ; Down
    mov ah, 0
    mov al, SCAN_CODE_DOWN_ARROW
    mov bx, cs
    mov dx, handle_down
    int 0xb3
    mov [cs:down_handler_segment], bx
    mov [cs:down_handler_offset], bx

    ; Up
    mov ah, 0
    mov al, SCAN_CODE_UP_ARROW
    mov bx, cs
    mov dx, handle_up
    int 0xb3
    mov [cs:up_handler_segment], bx
    mov [cs:up_handler_offset], bx

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
deregister_events:
    ; Escape
    mov ah, 0x80
    mov bx, [cs:escape_handler_segment]
    mov dx, [cs:escape_handler_offset]
    int 0xb3

    ; Left
    mov ah, 0x80
    mov bx, [cs:left_handler_segment]
    mov dx, [cs:left_handler_offset]
    int 0xb3

    ; Right
    mov ah, 0x80
    mov bx, [cs:right_handler_segment]
    mov dx, [cs:right_handler_offset]
    int 0xb3

    ; Down
    mov ah, 0x80
    mov bx, [cs:down_handler_segment]
    mov dx, [cs:down_handler_offset]
    int 0xb3

    ; Up
    mov ah, 0x80
    mov bx, [cs:up_handler_segment]
    mov dx, [cs:up_handler_offset]
    int 0xb3

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handler for escape key
;-------------------------------------------------------------------------------
handle_escape:
    cmp byte [cs:init_finished], TRUE
    jnz .done

    int 0x95 ; cls

    ; ToDo: Show commands (ctrl+e is exit)
    xchg bx, bx

    cli
    mov sp, [cs:stack_start] ; restore stack from program setup
    sti

    jmp deallocate_memory ; leave program

    .done:
        retf
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handler for left key
;-------------------------------------------------------------------------------
handle_left:
    cmp byte [cs:init_finished], TRUE
    jnz .done

    cmp byte [cs:mtx_col], 0
    jz .done

    dec byte [cs:mtx_col]
    dec word [cs:buffer_current_pos]

    ; Update screen
    call handle_display
    call update_cursor
    call redraw_cursor

    .done:
        retf
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handler for right key
;-------------------------------------------------------------------------------
handle_right:
    cmp byte [cs:init_finished], TRUE
    jnz .done

    mov al, [cs:page_cols]
    cmp [cs:mtx_col], al
    jnb .done

    mov ax, [cs:buffer_last_pos]
    cmp [cs:buffer_current_pos], ax
    jnb .done

    ; check if current character is crlf -> do not go right
    mov di, [cs:buffer_current_pos]
    cmp word [es:di], ASCII_CRLF
    jz .done
    cmp word [es:di], ASCII_LFCR
    jz .done

    inc byte [cs:mtx_col]
    inc word [cs:buffer_current_pos]

    ; Update screen
    call handle_display
    call update_cursor
    call redraw_cursor

    .done:
        retf
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handler for down key
;-------------------------------------------------------------------------------
handle_down:
    cmp byte [cs:init_finished], TRUE
    jnz .done

    mov ax, [cs:buffer_last_pos]
    cmp [cs:buffer_current_pos], ax
    jnb .done

    ; Check if end of screen reached
    mov al, [cs:page_lines]
    dec al
    cmp [cs:mtx_line], al
    jb .no_page_shift
        ; end of screen reached -> set page offset:
        xor ah, ah
        mov al, [cs:mtx_col] ; save col offset -> needed to calculate new buffer position in set_buffer_position_to_next_line
        push word [cs:buffer_current_pos] ; save current position
            ; Set current position to top of screen
            mov di, [cs:buffer_page_offset]
            add di, ax ; add offset from intput -> necessary for set_buffer_position_to_next_line.set_buffer_position_to_column_start
            mov [cs:buffer_current_pos], di
            ; Get offset to next line
            call set_buffer_position_to_next_line
            ; Set page offset to start of next line
            mov di, [cs:buffer_current_pos]
            mov [cs:buffer_page_offset], di
        pop word [cs:buffer_current_pos]
        mov [cs:mtx_col], al
    .no_page_shift:

    call set_buffer_position_to_next_line 

    ; Check if end of screen reached -> Do not increase line counter if so
    mov al, [cs:page_lines]
    dec al
    cmp [cs:mtx_line], al
    jz .skip_inc_line
        inc byte [cs:mtx_line]
    .skip_inc_line:

    ; Update screen
    call handle_display
    call update_cursor
    call redraw_cursor

    .done:
        retf
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handler for up key
;-------------------------------------------------------------------------------
handle_up:
    cmp byte [cs:init_finished], TRUE
    jnz .done

    cmp word [cs:buffer_current_pos], 0
    jz .done

    ; Check if at top of screen
    cmp byte [cs:mtx_line], 0
    ja .no_page_shift
        ; top of screen reached -> set page offset:
        xor ah, ah
        mov al, [cs:mtx_col] ; save col offset -> needed to calculate new buffer position in set_buffer_position_to_next_line
        push word [cs:buffer_current_pos] ; save current position
            ; Set current position to top of screen
            mov di, [cs:buffer_page_offset]
            add di, ax ; add offset from intput -> necessary for set_buffer_position_to_previous_line.set_buffer_position_to_column_start
            mov [cs:buffer_current_pos], di
            ; Get offset to previous line
            call set_buffer_position_to_previous_line
            ; Set page offset to start of previous line
            mov di, [cs:buffer_current_pos]
            mov [cs:buffer_page_offset], di
        pop word [cs:buffer_current_pos]
        mov [cs:mtx_col], al
    .no_page_shift:

    call set_buffer_position_to_previous_line

    ; Check if top of screen reached -> Do not decrease line counter if so
    cmp byte [cs:mtx_line], 0
    jz .skip_dec_line
        dec byte [cs:mtx_line]
    .skip_dec_line:

    ; Update screen
    call handle_display
    call update_cursor
    call redraw_cursor

    .done:
        retf
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Main loop
;-------------------------------------------------------------------------------
program_loop:
    call handle_display
    call handle_keyboard
    jmp program_loop
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handles the correct presentation of the entered data
;-------------------------------------------------------------------------------
handle_display:
    int 0x95 ; cls

    mov di, [cs:buffer_page_offset]
    xor cx, cx
    mov cl, [cs:page_lines]
    .loop_lines:
        push cx
            mov [cs:.line_position_in_stack], sp 
            xor cx, cx
            mov cl, [cs:page_cols]
            .loop_cols:
                .check_buffer_end:
                    cmp di, [cs:buffer_last_pos]
                    jb .check_crlf
                    call .last_byte ; adjust registers for last byte
                    jmp .check_nul ; then proceed with printing the last byte -> crlf is two bytes
                .check_crlf:
                    cmp word [es:di], ASCII_CRLF
                    jnz .check_lfcr
                    call .crlf
                    jmp .loop_cols_end
                .check_lfcr:
                    cmp word [es:di], ASCII_LFCR
                    jnz .check_nul
                    call .crlf
                    jmp .loop_cols_end
                .check_nul:
                    cmp byte [es:di], ASCII_NUL
                    jnz .check_other
                    call .nul
                    jmp .loop_cols_end
                .check_other:
                    call .other
                    jmp .loop_cols_end
                .loop_cols_end:
                    inc di
            loop .loop_cols
        pop cx
    loop .loop_lines

    xor ah, ah
    int 0x9b ; flip double buffer

    ret

    .last_byte:
        push cx ; remaining cols
            cli
            mov bp, sp
            mov sp, [cs:.line_position_in_stack]
            sti

            ; Adjust number of lines
            pop cx ; number of remaining lines
            mov cx, 1 ; this is the last line
            push cx ; put the adjusted lines on the stack again

            cli
            mov sp, bp
            sti
        pop cx ; remaining cols

        mov cx, 1 ; this is the last column

        ret

    .crlf:
        mov al, ASCII_LF
        int 0x90

        ; set the remaining cols to 1, so the current line ends immediately
        mov cx, 1

        ; increase di by 1 extra byte -> crlf are two bytes
        inc di

        ret

    .nul:
        mov al, ASCII_SP ; treat nul as space
        int 0x90 ; putch
        ret

    .other:
        mov al, [es:di]
        int 0x90 ; putch
        ret

    .line_position_in_stack dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Handles the keyboard entry
;-------------------------------------------------------------------------------
handle_keyboard:
    ; display cursor on correct position
    call update_cursor

    int 0xb5 ; clear keyboard buffer

    mov ah, 0xff
    int 0xb0 ; getch no putch

    cmp al, ASCII_BS
    jz .handle_backspace

    cmp word [cs:buffer_last_pos], BUFFER_SIZE
    jz .done ; if buffer is full, only accept backspace

    cmp al, ASCII_CR
    jz .handle_return

    ; Every other key
    call insert_from_al
    call increase_col

    .done:
        ret

    .handle_backspace:
        cmp word [cs:buffer_last_pos], 0
        jz .done ; nothing to delete

        ;cmp byte [cs:mtx_col], 0
        ;jz .done ; nothing to delete

        call delete_previous
        call decrease_col

        ret

    .handle_return:
        cmp word [cs:buffer_last_pos], BUFFER_SIZE - 1
        jz .done ; if buffer is full, only accept backspace

        mov ah, 0xff
        int 0xb0 ; get the remaining lf from the buffer

        call insert_crlf
        call increase_line
        mov byte [cs:mtx_col], 0 ; reset col

        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sets the cursor to the correct position
;-------------------------------------------------------------------------------
update_cursor:
    mov ah, 1
    xor bx, bx
    xor dx, dx
    mov bl, [cs:mtx_col]
    mov dl, [cs:mtx_line]
    int 0x99 ; set screen position
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Redraws the cursor to the current position
;-------------------------------------------------------------------------------
redraw_cursor:
    test byte [cs:video_mode], VIDEO_MODE_GFX_320x200
    jz .done ; cursor is already drawn in text mode

    mov al, [cs:cursor_symbol]
    int 0x90 ; putch

    xor ah, ah
    int 0x9b ; flip double buffer

    call update_cursor ; reset position

    .done:
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Inserts the character at the current position, shifting every other byte over the end pointer
;-------------------------------------------------------------------------------
insert_from_al:
    ; shift each byte one byte to the end, until the current position is reached
    mov di, [cs:buffer_last_pos]
    mov cx, [cs:buffer_last_pos]
    sub cx, [cs:buffer_current_pos]

    cmp cx, 0
    jz .insert_al

    .shift_to_end_loop:
        dec di ; di starts at offset for new character -> byte is 0 -> skip this byte
        mov ah, [es:di]
        mov [es:di + 1], ah
    loop .shift_to_end_loop

    ; there is now space for the character in al, store it
    .insert_al:
        mov di, [cs:buffer_current_pos]
        mov [es:di], al

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Increase the column -> check if page advance is necessary
;-------------------------------------------------------------------------------
increase_col:
    inc word [cs:buffer_current_pos]
    inc word [cs:buffer_last_pos]
    inc byte [cs:mtx_col]

    mov al, [cs:mtx_col]
    cmp al, [cs:page_cols]
    jz increase_line

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Increase the line -> check if page advance is necessary
;-------------------------------------------------------------------------------
increase_line:
    mov byte [cs:mtx_col], 0 ; reset col
    inc byte [cs:mtx_line]

    mov al, [cs:mtx_line]
    cmp al, [cs:page_lines]
    jz increase_page

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Increases the current page
;-------------------------------------------------------------------------------
increase_page:
    xor ax, ax
    mov al, [cs:page_cols]
    add [cs:buffer_page_offset], ax

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Inserts cr and lf to the current position
;-------------------------------------------------------------------------------
insert_crlf:
    mov al, ASCII_CR
    call insert_from_al

    inc word [cs:buffer_current_pos]
    inc word [cs:buffer_last_pos]

    mov al, ASCII_LF
    call insert_from_al

    inc word [cs:buffer_current_pos]
    inc word [cs:buffer_last_pos]

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Removes the current byte (or word, if current word is crlf) from the buffer 
;-------------------------------------------------------------------------------
delete_previous:
    mov di, [cs:buffer_current_pos]

    cmp word [es:di - 2], ASCII_CRLF
    jz .crlf
    cmp word [es:di - 2], ASCII_LFCR
    jz .crlf

    ; remove single byte
    call .shift_left
    ret

.crlf:
    ; remove two bytes
    call .shift_left
    call decrease_col
    mov di, [cs:buffer_current_pos]
    call .shift_left

    call set_buffer_position_to_previous_line

    dec byte [cs:mtx_line]

    ret

.shift_left:
    dec di ; di points to the location the next byte will be inserted, it is necessary to remove the current one

    mov cx, [cs:buffer_last_pos]
    sub cx, [cs:buffer_current_pos]

    cmp cx, 0
    jz .after_shift ; no shift necessary

    .loop_shift:
        mov ah, [es:di + 1]
        mov [es:di], ah
        inc di
    loop .loop_shift

    mov byte [es:di], 0 ; remove last byte

    .after_shift:

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sets the buffer_current_pos to the beginning of the current line
; Zeros mtx_col
;-------------------------------------------------------------------------------
set_buffer_position_to_column_start:
    xor ax, ax
    mov al, [cs:mtx_col]

    sub [cs:buffer_current_pos], ax

    mov byte [cs:mtx_col], 0

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sets the buffer_current_pos to the start of the previous line
; Zeros mtx_col
;
; 1. Set buffer_current_pos to star of line
; 2. Set buffer_current_pos to end of last line
; 3. Search for crlf within a maximum of 80 chars
; 4. Subtract count from buffer_current_pos
;-------------------------------------------------------------------------------
set_buffer_position_to_previous_line:
    call set_buffer_position_to_column_start

    cmp word [cs:buffer_current_pos], 0 ; check if first line already reached
    jz .done

    dec word [cs:buffer_current_pos] ; go to end of last line

    xor cx, cx ; offset to decrease from buffer_current_pos to get to line start
    mov bl, [cs:page_cols] 
    dec bl ; chars for full line
    mov di, [cs:buffer_current_pos]

    .loop_check_next_char:
        ; No more space
        cmp di, 0
        jz .decrease_buffer_current_pos

        ; Check if space for crlf
        cmp di, 1
        jbe .no_crlf

        ; Check if crlf found
        cmp word [es:di - 2], ASCII_CRLF
        jz .decrease_buffer_current_pos
        cmp word [es:di - 2], ASCII_LFCR
        jz .decrease_buffer_current_pos

        .no_crlf:

        inc cx
        dec di

        cmp cl, bl ; check for full line
        jz .decrease_buffer_current_pos
    jmp .loop_check_next_char

    .decrease_buffer_current_pos:
        sub [cs:buffer_current_pos], cx
        mov di, [cs:buffer_current_pos]

    .done:
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sets the buffer_current_pos to the start of the next line
; Zeros mtx_col
;
; 1. Set buffer_current_pos to star of line
; 2. Search for crlf within a maximum of 80 chars
; 3. Add count to buffer_current_pos
;-------------------------------------------------------------------------------
set_buffer_position_to_next_line:
    call set_buffer_position_to_column_start

    xor cx, cx ; offset to decrease from buffer_current_pos to get to line start
    mov bl, [cs:page_cols] ; number of chars for full line
    mov di, [cs:buffer_current_pos]

    .loop_check_next_char:
        ; No more space
        cmp di, 0xffff
        jz .increase_buffer_current_pos

        ; Check if space for crlf
        cmp di, 0xfffe
        jae .no_crlf

        ; Check if crlf found
        cmp word [es:di], ASCII_CRLF
        jz .crlf
        cmp word [es:di], ASCII_LFCR
        jz .crlf

        .no_crlf:

        inc cx
        inc di

        cmp cl, bl ; check for full line
        jz .increase_buffer_current_pos
    jmp .loop_check_next_char

    .crlf:
        add cx, 2 ; jump over crlf

    .increase_buffer_current_pos:
        add [cs:buffer_current_pos], cx
        mov di, [cs:buffer_current_pos]

    .done:
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Decrease the column
;-------------------------------------------------------------------------------
decrease_col:
    dec word [cs:buffer_current_pos]
    dec word [cs:buffer_last_pos]
    dec byte [cs:mtx_col]
    ret
;-------------------------------------------------------------------------------