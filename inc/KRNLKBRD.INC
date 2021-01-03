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
; TONY Kernel keyboard module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
%include "SCANCODE.INC"
%include "FUNCCODE.INC"
%include "ASCII.INC"
KEYBOARD_RELEASED_FLAG                  equ 0b10000000 ; MSB indicates that key was released
KEYBOARD_KEY_STATE_NOT_PRESSED          equ 0
KEYBOARD_KEY_STATE_PRESSED              equ 1
KEYBOARD_KEY_BUFFER_LENGTH              equ 32
KEYBOARD_FUNC_KEY_IDENT_OFFSET          equ 2
KEYBOARD_KEYMAP_ALTGR_MODIFIER_OFFSET   equ 4
KEYBOARD_KEYMAP_LINE_BYTES              equ 5
KEYBOARD_KEY_STATE_EXTENDED_OFFSET      equ 128

; Keyboard feedback
KEYBOARD_KEY_DETECTION_ERROR_1          equ 0x00
KEYBOARD_SELF_TEST_PASSED               equ 0xaa
KEYBOARD_ECHO_RESPONSE                  equ 0xee
KEYBOARD_ACKNOWLEDGE                    equ 0xfa
KEYBOARD_SELF_TEST_FAILURE_1            equ 0xfc
KEYBOARD_SELF_TEST_FAILURE_2            equ 0xfd
KEYBOARD_RESEND                         equ 0xfe
KEYBOARD_KEY_DETECTION_ERROR_2          equ 0xff
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
keyboard_video_mode_cursor db '_'
keyboard_key_state_table: times 256 db KEYBOARD_KEY_STATE_NOT_PRESSED ; stores "pressed" or "not pressed" for each key

keyboard_keymap: 
    times 2048 db 0 ; Space for the keymap

keyboard_key_buffer_entries db 0
keyboard_key_buffer: times KEYBOARD_KEY_BUFFER_LENGTH db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Initializes the keyboard
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
keyboard_init:
    ;xchg bx, bx

    ; Set scan code set 0xf0
    ; Set typematic rate and delay 0xf3
    ; Enable scanning 0xf4

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sets the intial keyboard keymap
;
; input: 
;   ds:si -> keyboard-map file name
;
; output:
;   none
;-------------------------------------------------------------------------------
keyboard_set_intial_keymap:
    push ds
    push si
        push cs
        pop ds
        mov al, 0
        mov si, .info_reading_keymap
        int 0x97
    pop si
    pop ds

    int 0x91 ; Display filename

    int 0xc1
    cmp ax, -1
    jz .critical_hang

    call keyboard_update_keymap
    call crlf

    ret

    .critical_hang:
        mov al, 4
        push cs
        pop ds
        mov si, .critical_keymap_does_not_exist
        int 0x97

    .hang_loop:
        cli
        hlt
    jmp .hang_loop

    .info_reading_keymap db "Reading keymap file ", 0
    .critical_keymap_does_not_exist db "Keymap does not exist! System hung up!", 0
;-------------------------------------------------------------------------------
; Updates the keyboard keymap
;
; input: 
;   ds:si -> keyboard-map file name
;
; output:
;   none
;-------------------------------------------------------------------------------
keyboard_update_keymap:
    ; Load keymap to keyboard_keymap
    push es
    push di
        push cs
        pop es
        mov di, keyboard_keymap
        int 0xc0
    pop di
    pop es

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Called on the keyboard interrupt -> Key action
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
keyboard_callback:
    pusha
    pushf

    xor ax, ax
    xor bx, bx ; Offset for key state table for the current scan code
    xor cx, cx ; Offset for key state table (0 for normal, 128 for extended keys)
    in al, 0x60 ; Read scancode from keyboard

    cmp al, KEYBOARD_EXTENDED_SCAN_CODE
    jne .no_extended_scan_code

    mov byte [cs:.previous_call_extended], 1
    jmp .no_buffer_write

    .no_extended_scan_code:
        cmp byte [cs:.previous_call_extended], 0
        je .key_to_table

        mov ah, KEYBOARD_EXTENDED_SCAN_CODE ; Extended code identifier into ah
        mov cx, KEYBOARD_KEY_STATE_EXTENDED_OFFSET

        mov byte [cs:.previous_call_extended], 0 ; reset flag for next keypress

    .key_to_table:
        ; Check if key is released
        mov dl, KEYBOARD_KEY_STATE_PRESSED
        test al, KEYBOARD_RELEASED_FLAG
        je .released_checked 
        mov dl, KEYBOARD_KEY_STATE_NOT_PRESSED
        sub bl, KEYBOARD_RELEASED_FLAG ; al will be added to bl and if key was released, 0x80 will be added to bl via al, so that the correct byte in keyboard_key_state_table is being reset

        .released_checked:
        add bl, al
        add bx, cx ; potential offset for extended scancodes
        mov byte [cs:keyboard_key_state_table + bx], dl ; write new key state

        test al, KEYBOARD_RELEASED_FLAG
        jne .no_buffer_write ; if released flag is set

    .check_buffer_write:
        ; Get matching scan code in MAP, or skip writing to buffer
        mov bx, word [cs:keyboard_keymap] ; First word indicates table start
        .check_code_map:
            cmp bx, word [cs:keyboard_keymap + 2] ; Second word indicates table end
            je .no_buffer_write ; keycode not found, no write

            mov dx, word [cs:keyboard_keymap + bx] ; keycode
            xchg dh, dl ; saved in reversed order
            cmp dx, ax ; ah indicates extended, al is keycode
            je .check_function_key

            add bx, KEYBOARD_KEYMAP_LINE_BYTES ; Next scan code
            jmp .check_code_map

        .check_function_key:
            ; No buffer write on function key
            mov dl, FUNC_KEY_IDENT_A
            mov dh, FUNC_KEY_IDENT_B
            cmp word [cs:keyboard_keymap + KEYBOARD_FUNC_KEY_IDENT_OFFSET + bx], dx
            jne .check_key_modifier ; if this is not a function key, check modifiers and add char to buffer

            ; if this is a function key check next entry, maybe there is another one that is not signed as function key
            add bx, KEYBOARD_KEYMAP_LINE_BYTES ; Next scan code
            jmp .check_code_map

        .check_key_modifier:
        ; altgr / alt + shift has highest priority, then shift, then no modifier
        add bx, KEYBOARD_KEYMAP_ALTGR_MODIFIER_OFFSET
        ; check for altgr 
        mov ah, FUNC_KEY_ALT_GR
        int 0xb2
        cmp al, 1
        je .write_buffer
        ; check for shift+alt
        mov ah, FUNC_KEY_ALT
        int 0xb2
        cmp al, 1
        jne .check_shift
        mov ah, FUNC_KEY_SHIFT
        int 0xb2
        cmp al, 1
        je .write_buffer
        mov ah, FUNC_KEY_SHIFT_R
        int 0xb2
        cmp al, 1
        je .write_buffer

        .check_shift:
        ; check for shift
        dec bx ; shift character is 1 before altgr character
        mov ah, FUNC_KEY_SHIFT
        int 0xb2
        cmp al, 1
        je .write_buffer
        mov ah, FUNC_KEY_SHIFT_R
        int 0xb2
        cmp al, 1
        je .write_buffer

        ; no key modifiers
        dec bx ; default character is 1 before shift character

        .write_buffer:
        cmp [cs:keyboard_keymap + bx], byte ASCII_NUL
        je .no_buffer_write ; Do not write if char is no ascii char
        cmp [cs:keyboard_key_buffer_entries], byte KEYBOARD_KEY_BUFFER_LENGTH
        jge .no_buffer_write ; Do not write if buffer is full
        mov dx, keyboard_key_buffer ; Get key buffer pointer
        add dl, [cs:keyboard_key_buffer_entries]
        mov al, [cs:keyboard_keymap + bx] ; write char to key buffer
        mov bx, dx
        mov [cs:bx], al
        inc byte [cs:keyboard_key_buffer_entries]

    .no_buffer_write:
        call kernel_acknowledge_hardware_interrupt

    .done:
        popf
        popa
        iret

    .previous_call_extended db 0 ; indicates if the previous callback returned with an extended identifier
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Get the oldest ascii char in the buffer / wait for a key press and return it
; Putchs the char to the screen and advances cursor position
; Removes the returned key from the buffer
;
; input: 
;   none
;
; output:
;   AL -> character
;-------------------------------------------------------------------------------
keyboard_getch:
    int 0x96
    cmp al, VIDEOMODE_ASCII_COLOR_80x25
    je .no_cursor

        ; Display cursor
        mov byte [cs:.display_cursor], 1
        mov al, [cs:keyboard_video_mode_cursor]
        int 0x90

    .no_cursor:
    sti ; do not block interrupts -> keyboard interrupt needs to work...
    cmp byte [cs:keyboard_key_buffer], 0
    je .no_cursor ;No key pressed, wait for keypress...

        ; remove cursor
        cmp byte [cs:.display_cursor], 1
        jne .no_remove_cursor ; carry is set, if cursor is displayed
        mov al, ASCII_BS
        int 0x90

    .no_remove_cursor:
    mov al, [cs:keyboard_key_buffer] ; Get key to return
    cmp al, RETURN
    je .insert_lf

    pusha

    mov bx, keyboard_key_buffer
    mov cx, KEYBOARD_KEY_BUFFER_LENGTH - 1
    ; Remove key from buffer -> Shift keys
    .shift_keys:
        mov dl, [cs:bx + 1]
        mov [cs:bx], dl
        inc bx
        loop .shift_keys

    mov dl, 0
    mov [cs:bx], dl ; clear last entry 

    dec byte [cs:keyboard_key_buffer_entries]

    popa
    jmp .done

    ; if return was pressed, insert lf to advance to the next line into the current buffer pointer -> next getch will return lf
    .insert_lf:
        mov byte [cs:keyboard_key_buffer], LINEFEED


    .done:
        cmp ah, 0xff
        je .ret

        int 0x90 ; display character
    .ret:
        iret

    .display_cursor db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Read line from keyboard and write it to the screen
; Saves into es:di and adds 0 terminator at the end
;
; input: 
;   cx -> buffer length + 1 terminator
;   es:di -> write buffer
;
; output:
;   none
;-------------------------------------------------------------------------------
keyboard_readln:
    pusha

    dec cx ; terminator

    mov dx, di
    mov bx, cx ; Remember maximum size

    .loop:
        mov ah, 0xff
        int 0xb0 ; getch no putch

        cmp al, 0x08 ; Backspace
        je .backspace

        cmp al, 0x0d ; Return
        je .return

        cmp cx, 0 ; No more bytes -> Only accept backspace and return
        je .loop

        int 0x90 ; putch
        stosb ; Save to di
        dec cx
        jmp .loop ; Give the user the chance to enter bs and cr

    .backspace:
        cmp bx, cx
        je .loop ; Beginning of input, do not delete data at a position befor di

        dec di
        mov byte [di], 0 ; Delete backspace character
        inc cx ; A new byte is free for data

        mov al, ASCII_BS
        int 0x90 ; putch

        jmp .loop

    .return:
        int 0xb0 ; getch lf remaining in buffer

        mov al, ASCII_CR
        int 0x90
        mov al, ASCII_LF
        int 0x90

        ; null-terminate the string 
        mov al, 0
        stosb

        popa

    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Checks if a specific function key is pressed
;
; input: 
;   AH -> See FUNCCODE.INC constants
;
; output:
;   al = 0x00: key not pressed
;   al = 0x01: key pressed
;   al = 0xff: key not in current keymap
;-------------------------------------------------------------------------------
keyboard_is_function_key_pressed:
    push dx
    push cx
    push bx

    ; Get matching func code in MAP
    mov bx, word [cs:keyboard_keymap] ; First word indicates table start
    .check_code_map:
        cmp bx, word [cs:keyboard_keymap + 2] ; Second word indicates table end
        je .done_no_entry ; funccode not found, no write

        mov dl, byte [cs:keyboard_keymap + KEYBOARD_KEYMAP_ALTGR_MODIFIER_OFFSET + bx] ; altgr modifier is also funccode identifier
        cmp dl, ah 
        je .check_special_key

        add bx, KEYBOARD_KEYMAP_LINE_BYTES ; Next scan code
        jmp .check_code_map

    .check_special_key:
        mov dl, FUNC_KEY_IDENT_A 
        mov dh, FUNC_KEY_IDENT_B
        cmp word [cs:keyboard_keymap + KEYBOARD_FUNC_KEY_IDENT_OFFSET + bx], dx ; check if entry is a function key entry
        je .check_extended

        ; Not a special key -> check for another entry with the correct funccode
        add bx, KEYBOARD_KEYMAP_LINE_BYTES ; Next scan code
        jmp .check_code_map        

    .check_extended:
        mov cx, KEYBOARD_KEY_STATE_EXTENDED_OFFSET ; Offset for extended scan code

        mov dx, [cs:keyboard_keymap + bx] ; get scan code
        xchg dh, dl ; scan code in reversed order

        cmp dh, KEYBOARD_EXTENDED_SCAN_CODE
        je .check_state_table

        xor cx, cx ; not an extended scan code -> offset is zero

    .check_state_table:
        mov al, 0 ; assume key is not set

        xor bx, bx
        mov bl, dl ; get scan code
        add bx, cx ; offset for extended codes in key state table
        cmp byte [cs:keyboard_key_state_table + bx], 0
        je .done

        mov al, 1 ; key was set
        jmp .done

    .done_no_entry:
        mov al, 0xff

    .done:
        pop bx
        pop cx
        pop dx
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

