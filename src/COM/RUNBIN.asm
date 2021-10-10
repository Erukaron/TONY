;-------------------------------------------------------------------------------
; MIT License
;
; Copyright (c) 2021 Erukaron
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
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
file_size    dw 0
file_segment dw 0
file_offset  dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Entrypoint
;-------------------------------------------------------------------------------
MAIN:
    ; Check for parameter
    cmp byte [cs:argc], 0
    jz no_param

    ; Check file size
    mov si, argv
    int 0xc1
    cmp ax, -1 ; file does not exist
    jz file_non_existant
    cmp dx, 0 ; file is bigger than 0xffff
    ja file_size_limit

    ; Reserve memory (512 Byte blocks)
    xor dx, dx ; remainder
    mov ax, bx ; size in bytes
    push ax
        mov bx, 512 ; sector size
        div bx
    pop ax
    add ax, dx ; if there is a remainder, increase size to fit the remainder
    int 0xb8
    cmp bx, -1 ; Not enough memory
    jz no_memory
    mov [cs:file_size], ax
    mov [cs:file_segment], bx
    mov [cs:file_offset], dx

    ; Load file to reserved memory
    mov es, bx
    mov di, dx
    xchg bx, bx
    int 0xc0
    jc load_error

    ; return address (if needed)
    push cs
    push return_address
    ; far call to loaded address
    push bx ; segment
    push dx ; offset
    retf

no_param:
    mov si, no_param_err
    mov al, 2 ; error
    int 0x97 ; prefix print
    jmp return_to_caller
    no_param_err db 'No parameter submitted!', ASCII_CR, ASCII_LF, 0

file_non_existant:
    mov si, file_non_existant_msg
    mov al, 2 ; error
    int 0x97 ; prefix print
    jmp return_to_caller
    file_non_existant_msg db 'File does not exist!', ASCII_CR, ASCII_LF, 0

file_size_limit:
    mov si, file_size_limit_msg
    mov al, 2 ; error
    int 0x97 ; prefix print
    jmp return_to_caller
    file_size_limit_msg db 'File exceeds size of 64K!', ASCII_CR, ASCII_LF, 0

no_memory:
    mov si, no_memory_msg
    mov al, 2 ; error
    int 0x97 ; prefix print
    jmp return_to_caller
    no_memory_msg db 'Not enough memory!', ASCII_CR, ASCII_LF, 0

load_error:
    mov si, load_error_msg
    mov al, 2 ; error
    int 0x97 ; prefix print
    jmp return_to_caller
    load_error_msg db 'Error while loading file!', ASCII_CR, ASCII_LF, 0

; Deallocate memory 
return_address:
    mov ax, [cs:file_size]
    mov bx, [cs:file_segment]
    mov dx, [cs:file_offset]
    int 0xb9

return_to_caller:
    ret
;-------------------------------------------------------------------------------