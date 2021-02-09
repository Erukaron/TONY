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
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push si
    xor ax, ax

    .loop:
        cmp byte [ds:si], 0
        je .done

        inc ax
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
;   ah -> mode (flags)
;       0 contains substring 
;       1 are strings equal
;       2 caseinsensitive
;   ds:si -> String to be checked
;   es:di -> Check string
;
; output:
;   carry flag -> Set if ds:si string does not contain es:di string, otherwise clear
;   ax -> start of es:di string inside ds:si string (relative position to start of string)
;-------------------------------------------------------------------------------
str_contains:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    pusha
    push es
    ; !! If adding pushes -> See .no_contain and .contains -> Changing values in stack

    mov [cs:.mode], ah

    mov bx, di ; for restoring di
    mov dx, si ; for calculation of relative start address on match

    .compare:
        cmp byte [ds:si], 0
        je .si_zero

        cmp byte [es:di], 0
        je .di_zero

        mov al, [es:di]
        mov ah, [ds:si]

        test byte [cs:.mode], 2
        jz .compare_casesensitive

        ; only check caseinsensitive, if al is a character A-Z or a-z
        cmp al, ASCII_A
        jl .compare_casesensitive 
        cmp al, ASCII_Z_LOWER
        jg .compare_casesensitive 
        cmp al, ASCII_Z
        jle .compare_case_in_sensitive
        cmp al, ASCII_A_LOWER
        jg .compare_casesensitive

        .compare_case_in_sensitive:
            cmp al, ah
            jg .compare_al_lower
            jl .compare_al_upper
            jz .compare_casesensitive

            .compare_al_lower:
                ; al is higher in value than ah -> possibly an lower case letter
                sub al, ASCII_A_LOWER - ASCII_A
                jmp .compare_casesensitive

            .compare_al_upper:
                ; al is lower in value than ah -> possibly an upper case letter
                add al, ASCII_A_LOWER - ASCII_A
                jmp .compare_casesensitive

        .compare_casesensitive:
            cmp al, ah
            jne .skip_si_restore_di

            inc si
            inc di

        jmp .compare

    .skip_si_restore_di:
        ; check if strings need to be equal, if so leave -> if the program reaches here, the strings are unequal
        test byte [cs:.mode], 1
        jnz .no_contain

        mov di, bx
        inc si
        jmp .compare

    .si_zero:
        cmp byte[es:di], 0
        je .di_zero
        jmp .no_contain

    .no_contain:
        ; set carry in stack
        push ss
        pop es
        mov di, sp
        or word [es:di + 2 + 14 + 6], 1 ; 2 for push es 14 for pusha 6 for offset to flags from int call

        xor ax, ax
        jmp .done

    .di_zero:
        ; Check if strings should be equal -> if so, check if si is also zero
        test byte [cs:.mode], 1
        jz .contains

        cmp byte [ds:si], 0
        jnz .no_contain

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

    .mode db 0
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

    pushf
    pusha

    cmp bx, dx
    jz .done ; Nothing to copy

    cmp bx, dx
    ja .error_start_greater

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
    jmp .done

    .error_start_greater:
        jmp .done

    .done:
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
;   ah -> mode
;       1: Remove all whitespace characters
;       2: Remove all multiple whitespace characters e.g. "   " -> " "
;   ds:si -> string to remove white space characters from
;
; output:
;   ds:si -> removed white space characters
;   cx -> Number of white spaces removed
;-------------------------------------------------------------------------------
str_remove_whitespace:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    pushf
    push ax
    push ds
    push si
    xor cx, cx
    cld

    test ah, 1
    jnz .loop_all

    .loop_multiples:
        cmp byte [ds:si], 0
        jz .done

        call .is_whitespace
        jnc .next_multiples

        ; character is whitespace -> remove every following whitespace
        inc si
        .loop_multiples_inner:
            cmp byte [ds:si], 0
            jz .done

            call .is_whitespace
            jnc .next_multiples ; if not a whitespace, look for the next one

            call .remove
            jmp .loop_multiples_inner ; Do NOT increase si -> This will skip the new character at si, written by .remove

            inc si
        jmp .loop_multiples_inner

        .next_multiples:
        inc si
    jmp .loop_multiples

    .loop_all:
        cmp byte [ds:si], 0
        jz .done

        call .is_whitespace
        jnc .no_remove

        call .remove
        jmp .loop_all ; Do NOT increase si -> This will skip the new character at si, written by .remove

        .no_remove:
        inc si
    jmp .loop_all

    .remove:
        inc cx
        push si

        ; shift every byte one position to the left, until zero termination is reached
        .shift_all_bytes:
            mov al, [ds:si + 1]
            mov [ds:si], al
            inc si

            cmp al, 0 
            jnz .shift_all_bytes

        pop si

        ret

    .is_whitespace:
        cmp byte [ds:si], ASCII_SP
        jz .set_carry
        cmp byte [ds:si], ASCII_TAB
        jz .set_carry
        cmp byte [ds:si], ASCII_LF
        jz .set_carry
        cmp byte [ds:si], ASCII_CR
        jz .set_carry

        clc ; no whitespace
        ret

        .set_carry:
            stc ; whitespace
            ret

    .done:
        pop si
        pop ds
        pop ax
        popf
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Terminates the string at the first occurence of the character (overwriting 
; the character)
;
; input:
;   ds:si -> string to terminate
;   dl -> Character to terminate on
;
; output:
;   none
;-------------------------------------------------------------------------------
str_terminate:
    sti ; enable hardware interrupts while processing string

    push si

    .loop:
        cmp byte [ds:si], 0
        je .str_terminate_done
        cmp byte [ds:si], dl
        je .terminate
        inc si
        jmp .loop

    .terminate:
        mov byte [ds:si], 0

    .str_terminate_done:
        pop si
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
