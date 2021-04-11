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
DISPLAY_GRAY_40x25_MAX_X    equ 39
DISPLAY_GRAY_40x25_MAX_Y    equ 24

DISPLAY_COLOR_80x25_MAX_X   equ 79
DISPLAY_COLOR_80x25_MAX_Y   equ 24

DISPLAY_GFX_FONT_WIDTH      equ 4
DISPLAY_GFX_FONT_HEIGHT     equ 6
DISPLAY_GFX_SCREEN_WIDTH    equ 320
DISPLAY_GFX_SCREEN_HEIGHT   equ 200
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
display_video_mode     db VIDEOMODE_BIOS_ASCII_GRAY_40x25

display_color          db 0x06 ; high nibble is background, low nibble foreground
display_info_color     db 0x09 ; high nibble is background, low nibble foreground
display_warning_color  db 0x0e ; high nibble is background, low nibble foreground
display_error_color    db 0x0c ; high nibble is background, low nibble foreground
display_critical_color db 0x03 ; high nibble is background, low nibble foreground
display_enable_blink   db FALSE ; enables blinking or high intensity color
display_current_x      db 0
display_current_y      db 0
display_max_x          db 79
display_max_y          db 24
display_min_x          db 0
display_min_y          db 0
display_cols           db 80
display_lines          db 25

display_font_array:
    times 512 db 0

; darker gray
display_gfx_bg_color     db 19
; amber 2
display_gfx_fg_color     db 43

display_gfx_fg_info_color     db 0x09 
display_gfx_fg_warning_color  db 0x2c 
display_gfx_fg_error_color    db 0x0c 
display_gfx_fg_critical_color db 0x04 
display_gfx_current_x    dw 0
display_gfx_current_y    db 0
display_gfx_max_x        dw 319
display_gfx_max_y        db 199
display_gfx_min_x        dw 0
display_gfx_min_y        db 0
display_gfx_cols         db 80
display_gfx_lines        db 33

display_double_buffer_enabled db FALSE
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
    mov al, VIDEOMODE_BIOS_ASCII_COLOR_80x25
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
;   AL -> Video mode (0b0010001 : Texmode 40x25, 0b0100010 : Textmode 80x25, 0b1000010 : Videomode 320x200)
;
; output:
;   none
;-------------------------------------------------------------------------------
display_switch_video_mode:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push ax

    mov ah, VIDEOMODE_BIOS_ASCII_GRAY_40x25
    mov [cs:display_video_mode], ah

    cmp al, VIDEOMODE_ASCII_GRAY_40x25
    jnz .check_color_text
    mov byte [cs:display_max_x], DISPLAY_GRAY_40x25_MAX_X
    mov byte [cs:display_cols], DISPLAY_GRAY_40x25_MAX_X + 1
    jmp .done

    .check_color_text:
    cmp al, VIDEOMODE_ASCII_COLOR_80x25
    jnz .check_gfx_mode
    mov ah, VIDEOMODE_BIOS_ASCII_COLOR_80x25
    mov [cs:display_video_mode], ah
    mov byte [cs:display_max_x], DISPLAY_COLOR_80x25_MAX_X
    mov byte [cs:display_cols], DISPLAY_COLOR_80x25_MAX_X + 1
    jmp .done

    .check_gfx_mode:
    cmp al, VIDEOMODE_GFX_320_200_COLOR
    jnz .done
    mov ah, VIDEOMODE_BIOS_GFX_320_200_COLOR
    mov [cs:display_video_mode], ah
    jmp .done

    .done:
    mov ah, 0
    mov al, [cs:display_video_mode]
    int 0x10

    int 0x95 ; cls

    pop ax
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Returns the current video mode
;
; input: 
;   none
;
; output:
;   AL -> Video mode (0b0010001 : Texmode 40x25, 0b0100010 : Textmode 80x25, 0b1000010 : Videomode 320x200)
;-------------------------------------------------------------------------------
display_get_video_mode:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_ASCII_GRAY_40x25
    jnz .check_color_text
    mov al, VIDEOMODE_ASCII_GRAY_40x25
    jmp .done

    .check_color_text:
    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_ASCII_COLOR_80x25
    jnz .check_gfx_mode
    mov al, VIDEOMODE_ASCII_COLOR_80x25
    jmp .done


    .check_gfx_mode:
    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    jnz .done
    mov al, VIDEOMODE_GFX_320_200_COLOR
    jmp .done

    .done:
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Clears the entire screen
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
display_clear_screen:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push es
    pusha

    mov al, [cs:display_min_x]
    mov [cs:display_current_x], al
    mov al, [cs:display_min_y]
    mov [cs:display_current_y], al
    mov ax, [cs:display_gfx_min_x]
    mov [cs:display_gfx_current_x], ax
    mov al, [cs:display_gfx_min_y]
    mov [cs:display_gfx_current_y], al

    ; Select characters and segments according to video mode
    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_ASCII_GRAY_40x25
    jnz .not_VIDEOMODE_ASCII_GRAY_40x25
        mov al, 0 ; character
        mov ah, [cs:display_color]

        push word GRAYSCALEDISPLAY_TEXT_SEGMENT
        pop es
        mov di, GRAYSCALEDISPLAY_TEXT_OFFSET

        mov cx, 0x2000
    .not_VIDEOMODE_ASCII_GRAY_40x25:

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_ASCII_COLOR_80x25
    jnz .not_VIDEOMODE_ASCII_COLOR_80x25
        mov al, 0 ; character
        mov ah, [cs:display_color]

        push word COLORDISPLAY_TEXT_SEGMENT
        pop es
        mov di, COLORDISPLAY_TEXT_OFFSET

        mov cx, 0x4000
    .not_VIDEOMODE_ASCII_COLOR_80x25:

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    jnz .not_VIDEOMODE_GFX_320_200_COLOR
        mov al, [cs:display_gfx_bg_color]
        mov ah, [cs:display_gfx_bg_color]

        push word COLORDISPLAY_GFX_SEGMENT
        pop es
        mov di, COLORDISPLAY_GFX_OFFSET

        mov cx, 0x8000
    .not_VIDEOMODE_GFX_320_200_COLOR:

    ; If using double buffer overwrite video buffer with double buffer
    cmp byte [cs:display_double_buffer_enabled], TRUE
    jnz .no_double_buffer ; No double buffering
        push word VIDEO_DOUBLE_BUFFER_SEGMENT
        pop es
        mov di, VIDEO_DOUBLE_BUFFER_OFFSET
    .no_double_buffer:
    
    .loop_clear_buffer:
        mov word [es:di], ax
        add di, 2
    loop .loop_clear_buffer

    .done:
        popa
        pop es
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
    push es
    push di
        push cs
        pop es
        mov di, display_font_array
        int 0xc0
    pop di
    pop es

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
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    je display_gfx_putch

    pusha
    push es

    ; set es to target memory (directly write to video memory or write to double buffer)
    cmp byte [cs:display_double_buffer_enabled], TRUE
    jnz .no_double_buffer
        push word VIDEO_DOUBLE_BUFFER_SEGMENT
        mov word [cs:.buffer_offset], VIDEO_DOUBLE_BUFFER_OFFSET
        jmp .after_buffer_select
    .no_double_buffer:
        push word COLORDISPLAY_TEXT_SEGMENT
        mov word [cs:.buffer_offset], COLORDISPLAY_TEXT_OFFSET
    .after_buffer_select:
    pop es

    mov [cs:.character], al

    cmp al, LINEFEED
    je .lf

    cmp al, RETURN
    je .cr

    cmp al, BACKSPACE
    je .bs

    cmp al, ASCII_FIRST_PRINTABLE_CHAR
    jae .lo_ok
        ; char not printable
        pop es
        popa
        mov al, ASCII_DOT
        jmp display_putch
    .lo_ok:

    cmp al, ASCII_LAST_PRINTABLE_CHAR
    jbe .hi_ok
        ; char not printable
        pop es
        popa
        mov al, ASCII_DOT
        jmp display_putch
    .hi_ok:

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
        pop es
        popa
        iret

    .lf:
        ; on line feed, cr is implicitly set
        mov bl, [cs:display_min_x]
        mov byte [cs:display_current_x], bl

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
        mov di, [cs:.buffer_offset]
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
        mov di, [cs:.buffer_offset]
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
        mov bl, [cs:display_min_x]
        mov byte [cs:display_current_x], bl
        jmp .display_putch_done

    .bs:
        mov bl, [cs:display_min_x]
        cmp byte [cs:display_current_x], bl
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
    .buffer_offset dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Puts a character to the screen and advances cursor position in graphics mode
;
; input: 
;   AL -> character
;
; output:
;   none
;-------------------------------------------------------------------------------
display_gfx_putch:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    pusha
    push es

    ; set es to target memory (directly write to video memory or write to double buffer)
    cmp byte [cs:display_double_buffer_enabled], TRUE
    jnz .no_double_buffer
        push word VIDEO_DOUBLE_BUFFER_SEGMENT
        mov word [cs:.buffer_offset], VIDEO_DOUBLE_BUFFER_OFFSET
        jmp .after_buffer_select
    .no_double_buffer:
        push word COLORDISPLAY_GFX_SEGMENT
        mov word [cs:.buffer_offset], COLORDISPLAY_GFX_OFFSET
    .after_buffer_select:
    pop es

    cmp al, LINEFEED
    je .lf

    cmp al, RETURN
    je .cr

    cmp al, BACKSPACE
    je .bs

    cmp al, ASCII_FIRST_PRINTABLE_CHAR
    jae .lo_ok
        ; char not printable
        pop es
        popa
        mov al, ASCII_DOT
        jmp display_gfx_putch
    .lo_ok:

    cmp al, ASCII_LAST_PRINTABLE_CHAR
    jbe .hi_ok
        ; char not printable
        pop es
        popa
        mov al, ASCII_DOT
        jmp display_gfx_putch
    .hi_ok:

    mov bx, [cs:display_gfx_current_x]
    add bx, DISPLAY_GFX_FONT_WIDTH - 1 ; at least four pixels for character
    mov dx, [cs:display_gfx_max_x]
    cmp bx, dx
    jna .no_insert_lf
        ; Not enough room to display char -> insert lf
        push ax
            mov al, LINEFEED
            int 0x90
        pop ax
    .no_insert_lf:

    xor ah, ah

    ; Same check is done before?
    ;cmp al, ASCII_LAST_PRINTABLE_CHAR ; if not printable, dont print
    ;ja .display_putch_done

    ; offset removed with new font files supporting extended ascii chars
    ;sub al, ASCII_FIRST_PRINTABLE_CHAR ; subtract offset
    shl ax, 1 ; multiply with 2
    mov si, display_font_array
    add si, ax
    mov dx, [cs:si]
    xchg dh, dl
    mov [cs:.font_character], dx

    mov bx, [cs:display_gfx_current_x]
    mov al, [cs:display_gfx_current_y]    

    push ax
    push bx
        add al, DISPLAY_GFX_FONT_HEIGHT - 1 ; select lowest line
        ; Clear lower line -> Line where g, j, q, p would be printed
        mov cx, DISPLAY_GFX_FONT_WIDTH - 1
        .clear_lowest_column:
            mov dl, [cs:display_gfx_bg_color]
            int 0x94
            inc bx
        loop .clear_lowest_column
    pop bx
    pop ax

    ; Draw pixels  
    test word [cs:.font_character], 0x8000
    jz .after_shift_down ; no shift down
    ; Shift down next character by one pixel
    inc al

    .after_shift_down:

    mov cx, DISPLAY_GFX_FONT_HEIGHT - 1
    push bp
    mov bp, 0x4000
    .loop_rows:
        push cx
        mov cx, DISPLAY_GFX_FONT_WIDTH - 1

        .loop_columns:
            test word [cs:.font_character], bp
            jz .no_pixel
                mov dl, [cs:display_gfx_fg_color]
                int 0x94
                jmp .after_draw
            .no_pixel:
                mov dl, [cs:display_gfx_bg_color]
                int 0x94
            .after_draw:
            shr bp, 1
            inc bx
        loop .loop_columns

        sub bx, DISPLAY_GFX_FONT_WIDTH - 1
        inc al

        pop cx
    loop .loop_rows
    pop bp

    add word [cs:display_gfx_current_x], DISPLAY_GFX_FONT_WIDTH

    ; update cursor position
    mov ax, [cs:display_gfx_current_x]
    cmp ax, [cs:display_gfx_max_x]
    ja .lf

    .display_putch_done:
        pop es
        popa
        iret

    .lf:
        ; on line feed, cr is implicitly set
        mov bx, [cs:display_gfx_min_x]
        mov word [cs:display_gfx_current_x], bx

        ; check if y limit is reached
        xor bx, bx
        mov bl, DISPLAY_GFX_FONT_HEIGHT
        shl bx, 1 ; multiply by 2 to check if there would be space for a character in the next line
        add bl, [cs:display_gfx_current_y]
        mov bh, [cs:display_gfx_max_y]
        cmp bl, bh
        jna .lf_no_shift
        
        ; not enough lines -> shift screen up one line

        ; times for loop
        xor ax, ax
        xor cx, cx
        mov ax, DISPLAY_GFX_SCREEN_HEIGHT
        sub ax, DISPLAY_GFX_FONT_HEIGHT ; first line is erased
        mov cx, DISPLAY_GFX_SCREEN_WIDTH
        mul cx
        mov cx, ax

        push cx
            ; Ignore first line with characters, as it will be overwritten
            xor ax, ax
            mov ax, DISPLAY_GFX_SCREEN_WIDTH
            mov cx, DISPLAY_GFX_FONT_HEIGHT
            mul cx
            mov di, [cs:.buffer_offset]
            add di, ax

            ; bx is later used to determine the new position of the pixel (6 rows above the current pixel)
            xor ax, ax
            mov ax, DISPLAY_GFX_SCREEN_WIDTH
            mul cx
            mov bx, ax
        pop cx

        .lf_shift:
            ; save pixel
            mov al, [es:di]

            ; get position above pixel
            push di
                sub di, bx

                ; write pixel to position above
                mov [es:di], al
            pop di

            ; next char
            inc di

            loop .lf_shift

        ; get first char of last line into di
        mov di, [cs:.buffer_offset]
        xor ax, ax
        xor cx, cx
        mov al, DISPLAY_GFX_SCREEN_HEIGHT
        sub al, DISPLAY_GFX_FONT_HEIGHT ; 33*80 would be the line under the display area
        mov cx, DISPLAY_GFX_SCREEN_WIDTH
        mul cx
        add di, ax

        ; times for loop
        xor cx, cx
        mov ax, DISPLAY_GFX_SCREEN_WIDTH
        mov cl, DISPLAY_GFX_FONT_HEIGHT
        mul cx
        mov cx, ax

        mov dl, [cs:display_gfx_bg_color]

        .lf_clear_last_line:
            mov [es:di], dl
            inc di
        loop .lf_clear_last_line

        jmp .display_putch_done
            
        ; shifting is not required, enough free lines
        .lf_no_shift:
            add byte [cs:display_gfx_current_y], DISPLAY_GFX_FONT_HEIGHT

            jmp .display_putch_done

    .cr:
        mov bx, [cs:display_gfx_min_x]
        mov word [cs:display_gfx_current_x], bx
        jmp .display_putch_done

    .bs:
        mov ax, [cs:display_gfx_current_x]
        mov bx, [cs:display_gfx_min_x]
        add bx, DISPLAY_GFX_FONT_WIDTH
        cmp ax, bx ; check if there is at least one character
        jb .display_putch_done

        ; Replace character with space
        sub word [cs:display_gfx_current_x], DISPLAY_GFX_FONT_WIDTH
        mov al, SPACE
        int 0x90

        ; Set x back one step
        sub word [cs:display_gfx_current_x], DISPLAY_GFX_FONT_WIDTH

        jmp .display_putch_done

    .font_character dw 0
    .buffer_offset  dw 0
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
    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    je display_gfx_update_cursor

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
; Updates the cursor position in graphics mode to the current variables
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
display_gfx_update_cursor:
    
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
    sti ; Allow hardware interrupts to occur while processing software interrupts
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
        xor ax, ax
        int 0x9b ; flip buffer

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
    sti ; Allow hardware interrupts to occur while processing software interrupts
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
    sti ; Allow hardware interrupts to occur while processing software interrupts
    pusha
    push es

    cmp [cs:display_video_mode], byte VIDEOMODE_BIOS_GFX_320_200_COLOR
    jne .done

    cmp al, [cs:display_gfx_max_y]
    ja .done

    cmp bx, [cs:display_gfx_max_x]
    ja .done

    ; set es to target memory (directly write to video memory or write to double buffer)
    cmp byte [cs:display_double_buffer_enabled], TRUE
    jnz .no_double_buffer
        mov cx, VIDEO_DOUBLE_BUFFER_SEGMENT
        mov di, VIDEO_DOUBLE_BUFFER_OFFSET
        jmp .after_buffer_select
    .no_double_buffer:
        mov cx, COLORDISPLAY_GFX_SEGMENT
        mov di, COLORDISPLAY_GFX_OFFSET
    .after_buffer_select:
    mov es, cx

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

;-------------------------------------------------------------------------------
; Prints a string with a prefix
;
; input: 
;   AL -> Prefix code
;       0: Info
;       1: Warning
;       2: Error
;       4: Critical
;   DS:SI -> 0 terminated string
;
; output:
;   none
;-------------------------------------------------------------------------------
display_prefix_print:
    sti ; Allow hardware interrupts to occur while processing software interrupts
    pusha 

    cmp al, 4
    jg .done

    push ds
    push si
    mov dh, [cs:display_gfx_bg_color]
    mov dl, [cs:display_gfx_fg_color]
    push dx
    mov bl, [cs:display_color]
    push bx
        push cs
        pop ds
        mov si, .prefix_open_char
        int 0x91

        cmp al, 0
        jnz .no_info
            mov dl, [cs:display_gfx_fg_info_color]
            mov [cs:display_gfx_fg_color], dl
            mov al, [cs:display_info_color]
            mov [cs:display_color], al
            mov si, .prefix_info
            int 0x91
            jmp .print
        .no_info:

        cmp al, 1
        jnz .no_warning
            mov dl, [cs:display_gfx_fg_warning_color]
            mov [cs:display_gfx_fg_color], dl
            mov al, [cs:display_warning_color]
            mov [cs:display_color], al
            mov si, .prefix_warning
            int 0x91
            jmp .print
        .no_warning:

        cmp al, 2
        jnz .no_error
            mov dl, [cs:display_gfx_fg_error_color]
            mov [cs:display_gfx_fg_color], dl
            mov al, [cs:display_error_color]
            mov [cs:display_color], al
            mov si, .prefix_error
            int 0x91
            jmp .print
        .no_error:

        cmp al, 4
        jnz .no_critical
            mov dl, [cs:display_gfx_fg_critical_color]
            mov [cs:display_gfx_fg_color], dl
            mov al, [cs:display_critical_color]
            mov [cs:display_color], al
            mov si, .prefix_critical
            int 0x91
            jmp .print
        .no_critical:

    .print:
        ; reset colors
        pop bx
        mov [cs:display_color], bl
        pop dx
        mov [cs:display_gfx_bg_color], dh
        mov [cs:display_gfx_fg_color], dl

        push cs
        pop ds
        mov si, .prefix_close_char
        int 0x91

        pop si
        pop ds

        int 0x91

    .done:
        popa
        iret

    .prefix_open_char db "[", 0
    .prefix_close_char db "] ", 0
    .prefix_info db "INFO", 0
    .prefix_warning db "WARNING", 0
    .prefix_error db "ERROR", 0
    .prefix_critical db "CRITICAL", 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Get current screen position
;
; input: 
;   AH -> Flags
;       0: Get position
;       1: (GFX mode) get text cursor referenced to line and column number instead of pixel
;
; output:
;   BX -> X-position
;   DX -> Y-position
;-------------------------------------------------------------------------------
display_get_screen_pos:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    jz .gfx_mode

    ; text mode
    xor bx, bx
    xor dx, dx
    mov bl, [cs:display_current_x]
    mov dl, [cs:display_current_y]
    jmp .done

    .gfx_mode:
    xor dx, dx
    mov bx, [cs:display_gfx_current_x]
    mov dl, [cs:display_gfx_current_y]

    test ah, 1
    jz .done

    ; Calculate column and line number from pixel location
    push ax
    push cx
        mov cl, DISPLAY_GFX_FONT_HEIGHT
        mov ax, dx
        xor dx, dx
        div cl
        mov dl, al

        push dx
            mov cx, DISPLAY_GFX_FONT_WIDTH
            mov ax, bx
            xor dx, dx
            div cx
            mov bx, ax
        pop dx
    pop cx
    pop ax

    .done:
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Set current screen position
;
; input: 
;   AH -> Flags
;       0: Get position
;       1: (GFX mode) set text cursor referenced to line and column number instead of pixel
;   BX -> X-position
;   DX -> Y-position
;
; output:
;   none
;-------------------------------------------------------------------------------
display_set_screen_pos:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    jz .gfx_mode

    ; text mode
    mov [cs:display_current_x], bl
    mov [cs:display_current_y], dl
    jmp .done

    .gfx_mode:
    push bx
    push dx
        test ah, 1
        jz .pixel

        ; Calculate screen position from line and column number
        push ax
        push cx
            mov cl, DISPLAY_GFX_FONT_HEIGHT
            mov ax, dx
            mul cl
            mov dl, al

            push dx
                mov cx, DISPLAY_GFX_FONT_WIDTH
                mov ax, bx
                mul cx
                mov bx, ax
            pop dx
        pop cx
        pop ax

        .pixel:
        mov [cs:display_gfx_current_x], bx
        mov [cs:display_gfx_current_y], dl
    pop dx
    pop bx

    .done:
        call display_update_cursor
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Set color
;
; input: 
;   GFX mode: 
;       ah -> bg color
;       al -> fg color
;   Text mode:
;       al -> colorcode
;           high nibble is bg
;           low nibble if fg
;
; output:
;   none
;-------------------------------------------------------------------------------
display_set_color:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
    jz .gfx_mode

    ; text mode
    mov [cs:display_color], al
    jmp .done

    .gfx_mode:
    mov [cs:display_gfx_bg_color], ah
    mov [cs:display_gfx_fg_color], al

    .done:
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Writes the contents of the video buffer to the video memory in text mode
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
display_text_flip_buffer:
    pusha
    pushf
    push es
    push ds
        push word COLORDISPLAY_TEXT_SEGMENT
        pop es
        push word VIDEO_DOUBLE_BUFFER_SEGMENT
        pop ds
        mov di, COLORDISPLAY_TEXT_OFFSET
        mov si, VIDEO_DOUBLE_BUFFER_OFFSET
        mov cx, 0x4000 ; *2 for size of video memory in text mode
        cld
        cli ; no iterrupts while swapping buffer
            rep movsw
        sti 
    pop ds
    pop es
    popf
    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Writes the contents of the video buffer to the video memory in gfx mode
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
display_gfx_flip_buffer:
    pusha
    pushf
    push es
    push ds
        push word COLORDISPLAY_GFX_SEGMENT
        pop es
        push word VIDEO_DOUBLE_BUFFER_SEGMENT
        pop ds
        mov di, COLORDISPLAY_GFX_OFFSET
        mov si, VIDEO_DOUBLE_BUFFER_OFFSET
        mov cx, 0x8000 ; *2 for size of video memory in gfx mode
        cld
        cli ; no iterrupts while swapping buffer
            rep movsw
        sti 
    pop ds
    pop es
    popf
    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Double buffer control:
;   Flip graphics buffer
;   Enable/Disable double buffering -> Clears the screen
;
; input: 
;   ah -> 0: Flip buffer
;         1: Enable double buffering
;         2: Disable double buffering
;
; output:
;   none
;-------------------------------------------------------------------------------
display_double_buffer_ctrl:
    sti ; enable hardware interrupts

    cmp ah, 0
    jz .flip

    cmp ah, 1
    jz .enable

    cmp ah, 2
    jz .disable

    jmp .done

    .flip:
        cmp byte [cs:display_double_buffer_enabled], FALSE
        jz .done ; Do not write to video memory, if double buffering is not enabled

        cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_ASCII_GRAY_40x25
        jnz .not_VIDEOMODE_ASCII_GRAY_40x25
            call display_text_flip_buffer
            jmp .upd_cursor
        .not_VIDEOMODE_ASCII_GRAY_40x25:

        cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_ASCII_COLOR_80x25
        jnz .not_VIDEOMODE_ASCII_COLOR_80x25
            call display_text_flip_buffer
            jmp .upd_cursor
        .not_VIDEOMODE_ASCII_COLOR_80x25:

        cmp byte [cs:display_video_mode], VIDEOMODE_BIOS_GFX_320_200_COLOR
        jnz .not_VIDEOMODE_GFX_320_200_COLOR
            call display_gfx_flip_buffer
            jmp .upd_cursor
        .not_VIDEOMODE_GFX_320_200_COLOR:

        .upd_cursor:
            call display_update_cursor
            jmp .done

    .enable:
        mov byte [cs:display_double_buffer_enabled], TRUE
        int 0x95 ; clear screen -> Update buffer with current screen color
        jmp .done

    .disable:
        mov byte [cs:display_double_buffer_enabled], FALSE
        int 0x95 ; clear screen -> Update video memory with current screen color
        jmp .done

    .done:
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
;    ret
;-------------------------------------------------------------------------------