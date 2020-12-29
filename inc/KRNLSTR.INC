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
; TONY Kernel string module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Get string length -> Number of chars to next terminator (terminator is not included in length)
; input:
;   ds:si -> String for length detection
; output:
;   ax -> Length of string
;-------------------------------------------------------------------------------
str_get_length:
    push si
    xor ax, ax

    .loop:
        inc ax

        cmp byte [ds:si], 0
        je .done

        inc si

        jmp .loop

    .done:
        pop si
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Initialize a string for a specified length with spaces and sets a terminator byte at the end
;
; input:
;   es:di -> String to initialize
;   cx -> length of string (one byte of this size is reserved for the terminator)
;
; output:
;   es:di -> Initialized string
;-------------------------------------------------------------------------------
str_init:
    push cx
    push di

    cmp cx, 0 
    je .done ; Do not overwrite data with terminator byte, if the buffer size is zero

    dec cx ; subtract 1 for the terminator that is always added at the end

    .loop:
        mov byte [es:di], al
        inc di

        loop .loop

    mov byte [es:di], 0

    .done:
        pop di
        pop cx
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Checks if a string contains another string
;
; input:
;   ah -> 0 contains substring, 1 are strings equal
;   ds:si -> String to be checked
;   es:di -> Check string
;
; output:
;   carry flag -> Set if ds:si string does not contain es:di string, otherwise clear
;   ax -> start of es:di string inside ds:si string (relative position to start of string)
;-------------------------------------------------------------------------------
str_contains:
    pusha
    push es
    ; !! If adding pushes -> See .no_contain and .contains -> Changing values in stack

    mov [cs:.equal_check], ah

    mov bx, di ; for restoring di
    mov dx, si ; for calculation of relative start address on match

    .compare:
        cmp byte [ds:si], 0
        je .si_zero

        cmp byte [es:di], 0
        je .contains

        mov al, [es:di]
        mov ah, [ds:si]
        cmp al, ah
        jne .skip_si_restore_di

        inc si
        inc di

        jmp .compare

    .skip_si_restore_di:
        ; check if strings need to be equal, if so leave -> if the program reaches here, the strings are unequal
        cmp byte [cs:.equal_check], 1
        jz .no_contain

        mov di, bx
        inc si
        jmp .compare

    .si_zero:
        cmp byte[es:di], 0
        je .contains
        jmp .no_contain

    .no_contain:
        ; set carry in stack
        push ss
        pop es
        mov di, sp
        or word [es:di + 2 + 14 + 6], 1 ; 2 for push es 14 for pusha 6 for offset to flags from int call

        xor ax, ax
        jmp .done

    .contains:
        ; set no carry in stack
        push ss
        pop es
        mov di, sp
        and word [es:di + 2 + 14 + 6], 0xfffe ; 2 for push es 14 for pusha 6 for offset to flags from int call

        mov ax, si
        sub ax, dx ; get relative start address

    .done:
        pop es
        popa
        iret

    .equal_check db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Copies a substring into another string
;
; input:
;   ds:si -> source string to create a copy from
;   bx -> relative start address
;   dx -> relative end address 
;
; output:
;   es:di -> target string buffer (will be 0 terminated, needs dx-bx + 1 bytes)
;-------------------------------------------------------------------------------
str_copy_substring:
    pushf
    pusha

    add si, bx ; start to copy from

    ; calculate loop iterations
    mov cx, dx
    sub cx, bx

    cld

    .copy:
        lodsb
        stosb
    loop .copy

    mov al, 0
    stosb

    popa
    popf
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Splits a string on the occurence of a specific character
; First part of the string remains in ds:si (will be terminated), second part is split into es:di
;
; input: 
;   ds:si -> String to split
;   ah -> 0 Split with termination of old string, 1 Split without termination of old string
;   ah -> 2 terminate second part
;   al -> split character
;   cx -> max split length
;   es:di -> Buffer to second part
;
; output:
;   es:di -> Buffer to second part
;-------------------------------------------------------------------------------
str_split:
    pushf
    pusha

    ; Get buffer size of target string
    ;push ds
    ;push es
    ;pop ds
    ;    push si
    ;    push di
    ;    pop si
    ;        push ax
    ;            int 0xd0
    ;            mov cx, ax
    ;            dec cx ; subtract 1 for zero terminator
    ;        pop ax
    ;    pop si
    ;pop ds

    clc

    .loop_compare:
        cmp byte [ds:si], al
        je .split

        cmp byte [ds:si], 0
        je .done

        inc si
        jmp .loop_compare

    .split:
        stc

        test ah, 1
        jnz .no_si_termination
        mov byte [ds:si], 0 ; null terminate the first part of the split string
        .no_si_termination:
        inc si

        .loop_split:
            cmp cx, 0 ; Max split length
            je .done 

            cmp byte [ds:si], 0
            je .done

            mov al, [ds:si]
            mov [es:di], al
            ; Zero the bit in si
            test ah, 1
            jnz .no_si_zero_filling
            mov byte [ds:si], 0
            .no_si_zero_filling:
            inc si
            inc di
            dec cx

            jmp .loop_split

    .done:
        test ah, 10b
        jz .no_terminate_di
        mov [es:di], byte 0

        .no_terminate_di:
            popa
            popf
            iret  
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Removes all white space characters from a string (space, tab, cr and lf)
;
; input: 
;   ds:si -> string to remove white space characters from
;
; output:
;   ds:si -> removed white space characters
;   cx -> Number of white spaces removed
;-------------------------------------------------------------------------------
str_remove_whitespace:
    push ds
    push si
    xor cx, cx
    cld

    .loop:
        cmp byte [ds:si], 0
        jz .done

        cmp byte [ds:si], ASCII_SP
        jz .remove
        cmp byte [ds:si], ASCII_TAB
        jz .remove
        cmp byte [ds:si], ASCII_LF
        jz .remove
        cmp byte [ds:si], ASCII_CR
        jz .remove

        inc si
    jmp .loop

    .remove:
        inc cx
        push si

        ; shift every byte one position to the left, until zero termination is reached
        .shift_bytes:
            mov al, [ds:si + 1]
            mov [ds:si], al
            inc si

            cmp al, 0 
            jnz .shift_bytes

        pop si

        jmp .loop ; Return to loop


    .done:
        pop si
        pop ds
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
