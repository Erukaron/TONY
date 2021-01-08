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
; TONY Kernel math module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Performs a 32-bit mathematical operation
;
; input: 
;   BP -> Operation 
;       0: Addition
;       1: Subtraction
;       2: Equality Check
;   DX:AX -> Number 1
;   CX:BX -> Number 2
;
; output:
;   DX:AX -> Result 
;   BP -> Error 
;       0x0000: No error
;       0x0001: Overflow
;       0xffff: Operation not specified
;
;       For equality check:
;           0b000: Not equal
;           0b001: Equal
;           0b010: DX:AX greater than CX:BX
;           0b100: DX:AX smaller than CX:BX
;-------------------------------------------------------------------------------
math_32_bit_operation:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push bx
    push cx

    cmp bp, 0
    jz .addition
    cmp bp, 1
    jz .subtraction
    cmp bp, 2
    jz .equality
    jmp .error

    .addition:
        mov bp, 0 ; No error

        add ax, bx
        jnc .add_no_overflow
            inc dx

        .add_no_overflow:
        add dx, cx

        jnc .done

        mov bp, 1 ; On overflow
        jmp .done

    .subtraction:
        mov bp, 0 ; No error

        sub ax, bx
        jnc .sub_no_overflow
            dec dx

        .sub_no_overflow:
        sub dx, cx

        jnc .done

        mov bp, 1 ; On overflow
        jmp .done

    .equality:
        cmp dx, cx
        ja .equality_greater
        jl .equality_lesser
        cmp ax, bx
        ja .equality_greater
        jl .equality_lesser
        jz .equality_equal

        .equality_greater:
            mov bp, 0b010
            jmp .done
        .equality_lesser:
            mov bp, 0b100
            jmp .done
        .equality_equal:
            mov bp, 0b000
            jmp .done

    .error:
        mov bp, 0xffff
        jmp .done

    .done: 
        pop cx
        pop bx
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
;    iret/ret
;-------------------------------------------------------------------------------