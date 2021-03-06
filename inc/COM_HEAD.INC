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
; COM header file
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%include "COMMON.INC"
org PSP_LENGTH
jmp __com_prg_init__
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Common com-variables
;-----------------------------------------------------------------------------
argc db 0
argv times 0x80 + 1 db 0
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Set everything up
;-----------------------------------------------------------------------------
__com_prg_init__:
    xor cx, cx
    mov cl, [cs:PSP_CMD_LINE_LENGTH_OFFSET]
    mov byte [ds:argc], cl
    mov si, PSP_CMD_LINE_OFFSET
    xor bx, bx
    mov dx, cx
    mov di, argv
    int 0xd3 ; Copy psp command line to argv

    ; Proceed with execution of com file
;-----------------------------------------------------------------------------