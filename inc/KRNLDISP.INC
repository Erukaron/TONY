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
; TONY Kernel display module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
display_video_mode   db 0x00

display_color        db 0x02 ; high nibble is background, low nibble foreground
display_enable_blink db FALSE ; enables blinking or high intensity color
display_current_x    db 0
display_current_y    db 0
display_max_x        db 79
display_max_y        db 24
display_min_x        db 0
display_min_y        db 0
display_cols         db 80
display_lines        db 25

display_font_array:
    times 0x200 db 0
display_gfx_max_x    dw 319
display_gfx_max_y    db 199
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

;-------------------------------------------------------------------------------
; Resets the video mode to kernel default
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
display_restore_default_kernel_video_mode:
    mov ah, 0
    mov al, VIDEOMODE_ASCII_COLOR_80x25
    mov [cs:display_video_mode], al
    int 0x10

    mov al, FALSE
    call display_set_blink_mode

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Sets the blinking / high intensity mode
;
; input: 
;   AL -> 0: intense background color 1: blink
;
; output:
;   none
;-------------------------------------------------------------------------------
display_set_blink_mode:
    push bx

    mov [ds:display_enable_blink], al
    mov bl, al
    mov ax, 0x1003
    int 0x10

    pop bx
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Switches the video mode
;
; input: 
;   AL -> Video mode (0 : Textmode 80x25, 1 : Videomode 320x200)
;
; output:
;   none
;-------------------------------------------------------------------------------
display_switch_video_mode:
    push ax

    mov ah, VIDEOMODE_ASCII_COLOR_80x25
    mov [cs:display_video_mode], ah

    cmp al, 0
    jnz .check_gfx_mode
    mov ah, VIDEOMODE_ASCII_COLOR_80x25
    mov [cs:display_video_mode], ah
    jmp .done


    .check_gfx_mode:
    cmp al, 1
    jnz .done
    mov ah, VIDEOMODE_GFX_320_200_COLOR
    mov [cs:display_video_mode], ah
    jmp .done

    .done:
    mov ah, 0
    mov al, [cs:display_video_mode]
    int 0x10

    pop ax
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Updates the font file
;
; input: 
;   ds:si -> font file name
;
; output:
;   none
;-------------------------------------------------------------------------------
display_update_font:
    ; Load keymap to keyboard_keymap
    mov di, display_font_array
    int 0xc0

    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Puts a character to the screen and advances cursor position
;
; input: 
;   AL -> character
;
; output:
;   none
;-------------------------------------------------------------------------------
display_putch:
    pusha
    push es

    ; set es
    push word COLORDISPLAY_TEXT_SEGMENT
    pop es

    mov [cs:.character], al

    cmp al, LINEFEED
    je .lf

    cmp al, RETURN
    je .cr

    cmp al, BACKSPACE
    je .bs

    ; calculate character location
    ; video memory is linear -> desired location is posX + posY * Number of cols (NOT maxX position!)
    xor ax, ax
    xor cx, cx
    mov al, [cs:display_current_y]
    mov cl, [cs:display_cols]
    mul cl
    mov cl, [cs:display_current_x]
    add ax, cx

    ; each entry in video memory is two bytes in size (character and color)
    shl ax, 1 ; multiply by 2

    ; write character
    mov di, ax
    mov al, [cs:.character]
    mov ah, [cs:display_color]
    mov [es:di], ax

    ; update cursor position
    mov al, [cs:display_current_x]
    cmp [cs:display_max_x], al
    je .lf
    inc byte [cs:display_current_x]

    .display_putch_done:
        call display_update_cursor
        pop es
        popa
        iret

    .lf:
        ; on line feed, cr is implicitly set
        mov byte [cs:display_current_x], 0

        ; check if y limit is reached
        mov al, [cs:display_current_y]
        cmp [cs:display_max_y], al
        jne .lf_no_shift
        
        ; not enough lines -> shift screen up one line

        ; times for loop
        xor ax, ax
        xor cx, cx
        mov al, [cs:display_lines]
        dec al ; first line is erased
        mov cl, [cs:display_cols]
        mul cl
        mov cx, ax

        ; get first char offset AFTER shift
        xor ax, ax
        mov al, [cs:display_cols]
        shl ax, 1 ; each entry in video memory is two bytes in size (character and color) -> multiply by 2
        mov di, COLORDISPLAY_TEXT_OFFSET
        add di, ax

        ; For subtraction of cols from di
        xor bx, bx
        mov bl, [cs:display_cols]
        shl bx, 1 ; each entry in video memory is two bytes in size (character and color) -> multiply by 2

        .lf_shift:
            ; save character
            mov ax, [es:di]

            ; get position above character
            push di
            sub di, bx

            ; write character to position above
            mov [es:di], ax
            pop di

            ; next char
            add di, 2

            loop .lf_shift

        ; get first char of last line
        mov di, COLORDISPLAY_TEXT_OFFSET
        xor ax, ax
        xor cx, cx
        mov al, [cs:display_lines]
        dec al ; 25*80 would be the line under the display area
        mov cl, [cs:display_cols]
        mul cl
        shl ax, 1 ; each entry in video memory is two bytes in size (character and color) -> multiply by 2
        add di, ax

        ; times for loop
        xor cx, cx
        mov cl, [cs:display_cols]

        mov dl, SPACE
        mov dh, [cs:display_color]

        .lf_clear_last_line:
            mov [es:di], dx
            add di, 2
            loop .lf_clear_last_line

        jmp .display_putch_done
            
        ; shifting is not required, enough free lines
        .lf_no_shift:
            inc byte [cs:display_current_y]

            jmp .display_putch_done

    .cr:
        mov byte [cs:display_current_x], 0
        jmp .display_putch_done

    .bs:
        cmp byte [cs:display_current_x], 0
        je .display_putch_done

        ; Replace character with space
        dec byte [cs:display_current_x] 
        call display_update_cursor
        mov al, SPACE
        int 0x90

        ; Set x back one step
        dec byte [cs:display_current_x] 

        jmp .display_putch_done

    .character db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Updates the cursor position to the current variables
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
display_update_cursor:
    pusha

    ; desired location is posX + posY * Number of cols (NOT maxX position!)
    xor ax, ax
    xor cx, cx
    mov al, [cs:display_current_y]
    mov cl, [cs:display_cols]
    mul cl
    mov cl, [cs:display_current_x]
    add ax, cx
    mov bx, ax

    ; Set low byte
    mov al, 0x0f
    mov dx, 0x03d4
    out dx, al

    mov al, bl 
    mov dx, 0x03d5
    out dx, al

    ; Set high byte
    mov al, 0x0e
    mov dx, 0x03d4
    out dx, al

    mov al, bh
    mov dx, 0x03d5
    out dx, al

    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Prints a null terminated string
;
; input: 
;   DS:SI -> 0 terminated string
;
; output:
;   none
;-------------------------------------------------------------------------------
display_print:
    pusha 

    .exec:
        ; Get string until 0
        mov al, byte [ds:si]
        cmp al, 0
        je .done

        int 0x90
        inc si
        jmp .exec

    .done:
        call display_update_cursor

        popa
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Prints a null terminated string to a specific location
;
; input: 
;   DS:SI -> 0 terminated string
;   AH -> Y Coord
;   AL -> X Coord
;
; output:
;   none
;-------------------------------------------------------------------------------
display_write:
    pusha

    ; Save current location
    mov bl, [cs:display_current_x]
    mov bh, [cs:display_current_y]

    ; Set new location
    mov [cs:display_current_x], al
    mov [cs:display_current_y], ah

    int 0x91 ; print

    ; Reset location
    mov [cs:display_current_x], bl
    mov [cs:display_current_y], bh

    popa
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Put pixel
;
; input: 
;   AL -> height
;   BX -> width
;   DL -> color
;
; output:
;   none
;-------------------------------------------------------------------------------
display_put_pixel:
    pusha
    push es

    cmp [cs:display_video_mode], byte VIDEOMODE_GFX_320_200_COLOR
    jne .done

    cmp al, [cs:display_gfx_max_y]
    ja .done

    cmp bx, [cs:display_gfx_max_x]
    ja .done

    mov cx, COLORDISPLAY_GFX_SEGMENT
    mov es, cx
    mov di, COLORDISPLAY_GFX_OFFSET

    ; x offset
    add di, bx

    push dx ; save color
    mov ah, 0
    mov cx, [cs:display_gfx_max_x]
    add cx, 1
    mul cx
    add di, ax ; y offset
    pop dx ; get color 

    mov [es:di], dl

    .done:
        pop es
        popa
        iret
;-------------------------------------------------------------------------------