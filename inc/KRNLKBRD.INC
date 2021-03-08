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
KEYBOARD_KEYMAP_TABLE_START_IDENT_OFFSET equ 0
KEYBOARD_KEYMAP_TABLE_END_IDENT_OFFSET  equ 2
KEYBOARD_KEYMAP_SCANCODE_OFFSET         equ 1
KEYBOARD_KEYMAP_ASCII_LO_OFFSET         equ 2
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

KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT      equ 0x00
KEYBOARD_EVENT_HANDLE_NEXT_OFFSET       equ 0x02
KEYBOARD_EVENT_HANDLE_FLAGS             equ 0x04
    KEYBOARD_EVENT_HANDLE_FLAGS_SHFT        equ 0x01
    KEYBOARD_EVENT_HANDLE_FLAGS_CTRL        equ 0x02
    KEYBOARD_EVENT_HANDLE_FLAGS_ALT         equ 0x04
    KEYBOARD_EVENT_HANDLE_FLAGS_ON_RELEASE  equ 0x40
    KEYBOARD_EVENT_HANDLE_FLAGS_IN_SERVICE  equ 0x80
KEYBOARD_EVENT_HANDLE_SCANCODE          equ 0x05
KEYBOARD_EVENT_HANDLE_TARGET_SEGMENT    equ 0x06
KEYBOARD_EVENT_HANDLE_TARGET_OFFSET     equ 0x08
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

keyboard_event_handler_segment_start dw 0
keyboard_event_handler_offset_start dw 0
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

        ; Beep 5 times
        xor ax, ax
        int 0x88
        int 0x88
        int 0x88
        int 0x88
        int 0x88

    .hang_loop:
        cli
        hlt
    jmp .hang_loop

    .info_reading_keymap db "Reading keymap file ", 0
    .critical_keymap_does_not_exist db "Keymap does not exist! System halt!", 0
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
    ;sti -> Do not reenable interrupts -> This is a hardware interrupt with priority

    pusha
    pushf

    xor ax, ax
    xor bx, bx ; Offset for key state table for the current scan code
    xor cx, cx ; Offset for key state table (0 for normal, 128 for extended keys)
    in al, 0x60 ; Read scancode from keyboard
    mov [cs:.event_in_service_scancode], al ; save scancode for event managing

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
        jge .no_buffer_write_full ; Do not write if buffer is full
        mov dx, keyboard_key_buffer ; Get key buffer pointer
        add dl, [cs:keyboard_key_buffer_entries]
        mov al, [cs:keyboard_keymap + bx] ; write char to key buffer
        push bx
            mov bx, dx
            mov [cs:bx], al
        pop bx
        inc byte [cs:keyboard_key_buffer_entries]
        jmp .no_buffer_write ; acknowledge hw interrupt

    .no_buffer_write_full:
        ; Beep on full buffer
        xor ax, ax
        int 0x88

    .no_buffer_write:
        call kernel_acknowledge_hardware_interrupt

    .done:
        ; Check keyboard events
        call .check_events

        popf
        popa
        iret

    ; Check event handler list to see if any event requests the pressed key
    .check_events:
        push ds

        ; set ds:si to first element
        push word [cs:keyboard_event_handler_segment_start]
        pop ds
        mov si, [cs:keyboard_event_handler_offset_start]

        .check_events_loop:
            ; Check if scan code matches
            mov dl, [ds:si + KEYBOARD_EVENT_HANDLE_SCANCODE]
            cmp dl, byte [cs:.event_in_service_scancode]
            jnz .next_entry

            ; Check flags
            mov dl, [ds:si + KEYBOARD_EVENT_HANDLE_FLAGS]

            ; Check if event is already in service
            test dl, KEYBOARD_EVENT_HANDLE_FLAGS_IN_SERVICE
            jnz .next_entry

            ; Check modifier keys 
            test dl, KEYBOARD_EVENT_HANDLE_FLAGS_SHFT
            jz .event_skip_shift_check
                mov ah, FUNC_KEY_SHIFT
                int 0xb2
                cmp al, 1
                jnz .next_entry ; skip, if requested modifier key is not pressed
            .event_skip_shift_check:

            test dl, KEYBOARD_EVENT_HANDLE_FLAGS_ALT
            jz .event_skip_alt_check
                mov ah, FUNC_KEY_ALT
                int 0xb2
                cmp al, 1
                jnz .next_entry ; skip, if requested modifier key is not pressed
            .event_skip_alt_check:

            test dl, KEYBOARD_EVENT_HANDLE_FLAGS_CTRL
            jz .event_skip_ctrl_check
                mov ah, FUNC_KEY_CTRL
                int 0xb2
                cmp al, 1
                jnz .next_entry ; skip, if requested modifier key is not pressed
            .event_skip_ctrl_check:

            ; Mark event as in service
            or byte [ds:si + KEYBOARD_EVENT_HANDLE_FLAGS], KEYBOARD_EVENT_HANDLE_FLAGS_IN_SERVICE

            cld

            push ds
            push es
            pushf

            ; return address
            push cs
            push .event_return_point

            push word [ds:si + KEYBOARD_EVENT_HANDLE_TARGET_SEGMENT]
            push word [ds:si + KEYBOARD_EVENT_HANDLE_TARGET_OFFSET]

            sti ; Allow interrupts while processing the event
            retf ; jump to target address

            .event_return_point:
            cli ; Disable interrupts after event has been handled

            popf
            pop es
            pop ds

            ; Event is no longer in service
            mov dl, KEYBOARD_EVENT_HANDLE_FLAGS_IN_SERVICE
            not dl
            and byte [ds:si + KEYBOARD_EVENT_HANDLE_FLAGS], dl

            .next_entry:
                cmp word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT], 0
                jz .check_events_done

                push word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT]
                push word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET]
                pop si
                pop ds
                jmp .check_events_loop
            
        .check_events_done:
            pop ds
            ret

    .previous_call_extended db 0 ; indicates if the previous callback returned with an extended identifier
    .event_in_service_scancode db 0
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

    mov byte [cs:.display_cursor], 0

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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
; Keyboard event registration
; Keyboard event handling code needs to exit with a retf instruction
;
; input:
;   ah -> Flags
;       (Modifiers)
;       1 : Shift
;       2 : Ctrl
;       4 : Alt
;       (Options)
;     128 : de-register the event -> bx:dx is returned event handler
;   al -> Scancode (extended scan codes are not supported!) (msb is 1 for event on key release instead of key down)
;   bx:dx -> Target address (event handler)
; output:
;   bx:dx -> Event handler (for deregistering the event)
;-------------------------------------------------------------------------------
; Event handle:
;   0x00 (2 Bytes) Next element segment
;   0x02 (2 Bytes) Next element offset
;   0x04 (1 Byte)  Flags
;       (Modifiers)
;       1 : Shift
;       2 : Ctrl
;       4 : Alt
;       (Options)
;     128 : event in service -> Target already called and not yet returned
;   0x05 (1 Byte)  Scancode / Function key code
;   0x06 (2 Bytes) Target segment
;   0x08 (2 Bytes) Target offset
;   0x0a (6 Bytes) Unused
;-------------------------------------------------------------------------------
keyboard_event_registrator:
    ;sti ; Do not allow interrupts -> Keyboard interrupt will then call to an undefined list element

    push es
    push ds
    push di
    push si

    test ah, 0b10000000
    jnz .deregister

    mov [cs:.target_segment], bx
    mov [cs:.target_offset], dx

    ; register event -> create new variable
    push ax
        mov ax, 16
        int 0xb8 ; Reserve 16 bytes
        mov es, bx
        mov di, dx
    pop ax

    mov [es:di + KEYBOARD_EVENT_HANDLE_FLAGS], ah
    mov [es:di + KEYBOARD_EVENT_HANDLE_SCANCODE], al
    push bx
    push dx
        mov bx, [cs:.target_segment]
        mov dx, [cs:.target_offset]
        mov [es:di + KEYBOARD_EVENT_HANDLE_TARGET_SEGMENT], bx
        mov [es:di + KEYBOARD_EVENT_HANDLE_TARGET_OFFSET], dx
    pop dx
    pop bx

    ; check if there are already event handlers
    cmp word [cs:keyboard_event_handler_segment_start], 0
    jz .create_first_handler

    ; append handler
    call .get_last_element
    mov [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT], es
    mov [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET], di
    jmp .done

    .create_first_handler:
        mov word [cs:keyboard_event_handler_segment_start], es
        mov word [cs:keyboard_event_handler_offset_start], di
        jmp .done

    ; loads the last element into ds:si
    ; at least one element needs to exist
    .get_last_element:
        mov ds, [cs:keyboard_event_handler_segment_start]
        mov si, [cs:keyboard_event_handler_offset_start]
        .get_last_element_next:
            cmp word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT], 0
            jz .get_last_element_done

            push word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT]
            push word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET]
            pop si
            pop ds
            jmp .get_last_element_next

        .get_last_element_done:
            ret

    .deregister:
        cmp word [cs:keyboard_event_handler_segment_start], 0
        jz .done ; There is no list entry -> end without deallocating anything

        ; Check if first entry is the searched one
        mov ds, [cs:keyboard_event_handler_segment_start]
        mov si, [cs:keyboard_event_handler_offset_start]
        mov ax, ds
        cmp ax, bx
        jnz .deregister_search_loop
        cmp si, dx
        jnz .deregister_search_loop

        ; First element is requested element to deregister
        mov ax, 16
        ; bx and dx already set
        int 0xb9 ; deallocate 16 bytes

        mov word [cs:keyboard_event_handler_segment_start], 0
        mov word [cs:keyboard_event_handler_offset_start], 0

        jmp .done

        ; Search for list element with segment:offset = bx:dx
        .deregister_search_loop:
            ; Previous element -> used to chain the previous to the next element, unchaining the searched entry
            push ds
            pop es
            mov di, si

            ; Get next element
            push word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT]
            push word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET]
            pop si
            pop ds

            mov ax, ds
            cmp ax, 0
            jz .done ; Last element checked -> Element to deallocate not inside list

            cmp ax, bx
            jnz .deregister_search_loop
            cmp si, dx
            jnz .deregister_search_loop

        ; Element to deregister found and loaded to ds:si, previous element loaded to es:di
        ; Check if element to deregister is the last element
        cmp word [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT], 0
        jz .deallocate_last

        ; Rechain the previous element (es:di) to the element after the one being deallocated
        push bx
        push dx
            mov bx, [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT]
            mov dx, [ds:si + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET]
            mov [es:di + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT], bx
            mov [es:di + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET], dx
        pop dx
        pop bx

        jmp .after_rechain

        ; remove chain link in previous entry
        .deallocate_last:
            mov word [es:di + KEYBOARD_EVENT_HANDLE_NEXT_SEGMENT], 0
            mov word [es:di + KEYBOARD_EVENT_HANDLE_NEXT_OFFSET], 0

        ; deallocate element in bx:dx
        .after_rechain:
            mov ax, 16
            ; bx and dx already set
            int 0xb9 ; deallocate 16 bytes
            jmp .done
       
    .done:
        pop si
        pop di
        pop ds
        pop es
        iret

    .target_segment dw 0
    .target_offset dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Returns the scancode for an ascii character
;
; input:
;   al -> ASCII character
;
; output:
;   al -> Scancode 
;       (0x00 if ascii character not in current keymap)
;       (only works for non extended scan codes!)
;-------------------------------------------------------------------------------
keyboard_ascii_to_scancode:
    push ds
    push si
    push bx
    push cx
    push dx

    mov ch, ah ; backup ah -> reset ah later without using the stack
    mov ah, al ; save character in ah

    push cs
    pop ds
    mov si, keyboard_keymap

    mov bx, [ds:si + KEYBOARD_KEYMAP_TABLE_END_IDENT_OFFSET]

    ; Load table start to ds:si
    mov dx, [ds:si + KEYBOARD_KEYMAP_TABLE_START_IDENT_OFFSET]
    add si, dx

    .loop:
        ; skip extended scan codes
        cmp byte [ds:si], KEYBOARD_EXTENDED_SCAN_CODE
        jz .next_entry

        ; save current scancode
        mov al, [ds:si + KEYBOARD_KEYMAP_SCANCODE_OFFSET]

        ; check every entry for one scan code
        cmp [ds:si + KEYBOARD_KEYMAP_SCANCODE_OFFSET + 1], ah ; ascii lower
        jz .done
        cmp [ds:si + KEYBOARD_KEYMAP_SCANCODE_OFFSET + 2], ah ; ascii higher
        jz .done
        cmp [ds:si + KEYBOARD_KEYMAP_SCANCODE_OFFSET + 3], ah ; alt_gr modifier
        jz .done

        ; load next entry (if any)
        .next_entry:
            cmp si, bx
            jnae .return_not_found

            add si, KEYBOARD_KEYMAP_LINE_BYTES ; next table entry
            jmp .loop

    .return_not_found:
        mov al, 0x00
        jmp .done

    .done:
        mov ah, ch ; reset ah
        pop dx
        pop cx
        pop bx
        pop si
        pop ds
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Clear keyboard buffer
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
keyboard_clear_buffer:
    cli ; Allow hardware interrupts to occur while processing software interrupts

    pusha
    push es
        mov ax, cs
        mov es, ax
        mov di, keyboard_key_buffer

        mov cx, KEYBOARD_KEY_BUFFER_LENGTH
        xor ax, ax
        cld

        .clear_buffer:
            stosb
        loop .clear_buffer

        mov byte [cs:keyboard_key_buffer_entries], 0
    pop es
    popa

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
;    sti ; Allow hardware interrupts to occur while processing software interrupts
;    iret / ret
;-------------------------------------------------------------------------------

