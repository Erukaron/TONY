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
; MLT - Assembler
; (No registers preserved!)
;
; Operation:
; Enter q to exit
; Current offset is shown with a prompt to enter mnemonic from list of vaild instructions
;   -> for valid instructions the written bytes are displayed and after that the mnemonic
;   -> for invalid instructions a '?' is displayed and nothing is written to memory 
;
; input:
;   ES -> Segment to start from
;   DI -> Offset to start from
;
; output:
;   none
;-------------------------------------------------------------------------------
mlt_assembler:
    .INPUT_BUFFER_LENGTH    equ 48

    push es
    push di
    pop word [cs:.next_offset]
    pop word [cs:.next_segment]

    mov ax, cs
    mov ds, ax
    mov es, ax

    .input_loop:
        ; Display crlf
        mov al, ASCII_LF
        int 0x90

        ; Display current segment:offset
        mov dx, es
        shr dx, 12 ; dl contains most significant nibble of es
        call .display_hex_from_dl
        mov dx, es
        shl dx, 4
        add dx, di ; dx contains offset
        mov bx, dx
        shr dx, 12 ; most significant nibble of offset
        call .display_hex_from_dl
        mov dx, bx
        shr dx, 8 ; second nibble of offset
        and dh, 0x0f
        call .display_hex_from_dl
        mov dx, bx
        shr dx, 4 ; third nibble of offset
        and dh, 0x0f
        call .display_hex_from_dl
        mov dx, bx
        and dh, 0x0f
        call .display_hex_from_dl

        ; Display ": "
        mov si, .prompt_string
        int 0x91

        ; Get mnemonic
        mov di, .input_buffer
        mov cx, .INPUT_BUFFER_LENGTH
        mov al, 0 
        int 0xd1 ; initialize string
        int 0xb1 ; read string

        ; Check for exit
        mov ah, 0b11 ; string equal, ignore case
        mov si, .exit_request
        int 0xd2
        jnc .done ; strings equal

        ; assemble code
        push es
        pop ds
        push di
        pop si
        mov ax, [cs:.next_segment]
        mov es, ax
        mov di, [cs:.next_offset]
        call assemble
        mov [cs:.assembly_error], ah

        ; Adjust next position
        xor ah, ah
        add [cs:.next_offset], ax

        ; Set text pointer to x = 7 (start mnemonic) -> "SOOOO: <mnemonic>"
        mov ah, 1
        int 0x98  ; get cursor
        sub bx, 7 ; x offset
        int 0x99  ; set cursor

        cmp byte [cs:.assembly_error], 0
        jz .input_loop_no_error

        ; on assembly error: 
            ; display "? " followed by mnemonic
            jmp .input_loop ; start over
        .input_loop_no_error: ; on non assembly error: 
            ; write code to es:di
            ; display "<assembled hex> " followed by mnemonic
            jmp .input_loop ; start over

    .done:
        ret

    .display_hex_from_dl:
        int 0xe4
        mov al, dh
        int 0x90
        mov al, dl
        int 0x90
        ret

    .input_buffer times .INPUT_BUFFER_LENGTH db 0
    .prompt_string db ": ", 0
    .exit_request db "q", 0
    .next_segment dw 0
    .next_offset  dw 0
    .assembly_error db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Assembles the line of text
; 
; input:
;   DS:SI -> Null-terminated string to assemble (one line at a time)
;   ES:DI -> Offset to write bytes to
;
; output:
;   AH      -> Error code
;   AL      -> No. of assembled bytes
;   ES:DI   -> Assembled code
;
; Valid instructions:
;   push ax
;   pop  ax
;   push cs
;   push ds
;   pop  ds
;   push es
;   pop  es
;   push ss
;   pop  ss
;   push sp
;   pop  sp
;   push bp
;   pop  bp
;   push si
;   pop  si
;   push di
;   pop  di
;   pushf
;   popf
;   mov ax, #imm
;   mov ax, $memory
;   mov $memory, ax
;   mov ah, #imm
;   mov ah, $memory
;   mov $memory, ah
;   mov al, #imm
;   mov al, $memory
;   mov $memory, al
;   mov ax, reg
;   mov reg, ax
;   mov ax, [si]    : mov ax, [ds:si]
;   mov ah, [si]    : mov ah, [ds:si]
;   mov al, [si]    : mov al, [ds:si]
;   mov [di], ax    : mov [es:di], ax
;   mov [di], ah    : mov [es:di], ah
;   mov [di], al    : mov [es:di], al
;   mov dx, ax
;   xchg ah, al
;   int #imm
;   call #imm       : call cs:#imm
;   ret
;   retf
;   cmp ax, #imm
;   cmp ah, #imm
;   cmp al, #imm
;   cmp ax, $memory
;   cmp ah, $memory
;   cmp al, $memory
;   cmp ax, [si]
;   cmp ah, [si]
;   cmp al, [si]
;   jmp #imm
;   jmpf #imm
;   ja #imm
;   jae #imm
;   jb #imm
;   jz #imm
;   jnz #imm
;   jc #imm
;   jo #imm
;   add ax, #imm
;   add ax, $memory
;   add ax, [si]
;   add ah, #imm
;   add ah, $memory
;   add ah, [si]
;   add al, #imm
;   add al, $memory
;   add al, [si]
;   add si, ax
;   add di, ax
;   sub ax, #imm
;   sub ax, $memory
;   sub ax, [si]
;   sub ah, #imm
;   sub ah, $memory
;   sub ah, [si]
;   sub al, #imm
;   sub al, $memory
;   sub al, [si]
;   sub si, ax
;   sub di, ax
;   db #imm
;   dw #imm
;   or al, #imm
;   or al, $memory
;   and al, #imm
;   and al, $memory
;   not al
;   xor al, #imm
;   xor al, $memory
;   inc ax
;   inc ah
;   inc al
;   inc si
;   inc di
;   dec ax
;   dec ah
;   dec al
;   dec si
;   dec di
;   out ah, dx
;   out al, dx
;   in ah, dx
;   in al, dx
;   nop
;   hlt
;   clc
;   stc
;   cli
;   sti
;   cld
;   std
;   shr ax
;   shr ah
;   shr al
;   shl ax
;   shl ah
;   shl al
;-------------------------------------------------------------------------------
assemble:
    xor ax, ax
    mov byte [cs:.assembly_error], 0

    .done: 
        mov ah, [cs:.assembly_error]
        ret

    .push:
        ret

    .pop:
        ret

    .assembly_error db 0

    ; Symbol list
    .sym_push db .push, 'push', 0
    .sym_pop db .pop, 'pop', 0
;-------------------------------------------------------------------------------