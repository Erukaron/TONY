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
; TONY Kernel memory module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
MEMORY_SMALLEST_UNIT    equ 16
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
memory_master_map: ; 4 kb -> Each bit represents 512/(4*8)=16 Bytes
    times 0x0200 db 0xff ; Kernel ->  64kB ( 64kB / 16B / 8b)
    times 0x0200 db 0xff ; Stack  ->  64kB ( 64kB / 16B / 8b)
    times 0x0c00 db 0x00 ; Free   -> 384kB (384kB / 16B / 8b)
memory_master_map_end:
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
;    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Reserves memory (in 16-byte units)
;
; input: 
;   AX -> Number of bytes to reserve
;
; output:
;   BX -> Segment
;   DX -> Offset
;   BX = DX = 0xffff -> not enough memory
;-------------------------------------------------------------------------------
memory_reserve_bytes:
    push ax
    push cx
    push si
    push di
    push ds
    push es

    cmp ax, 0
    je .not_enough_memory

    mov [cs:.requested_bytes], ax

    push cs
    pop ds
    push cs
    pop es

    mov di, 0xffff ; first address with free memory
    mov bx, 0 ; number of contingous bytes found
    mov ch, 0b10000000 ; start bit -> used to identify first free byte unit
    mov si, memory_master_map
    .search:
        cmp si, memory_master_map_end
        je .not_enough_memory

        mov cl, 0b10000000 ; test bit -> used in the loop and decreased every iteration
        mov dl, [ds:si]
        .test_bit:
            test dl, cl
            jnz .test_bit_reserved
            jmp .test_bit_ok

        .test_bit_reserved:
            mov bx, 0
            mov di, 0xffff ; reset memory chain
            shr cl, 1
            cmp cl, 0
            je .test_bit_done
            jmp .test_bit

        .test_bit_ok:
            cmp di, 0xffff
            jne .skip_origin_write
            mov di, si
            mov ch, cl

            .skip_origin_write:
            add bx, MEMORY_SMALLEST_UNIT
            cmp bx, ax
            jge .found

            shr cl, 1
            cmp cl, 0
            je .test_bit_done
            jmp .test_bit

        .test_bit_done:
        inc si
        jmp .search

    ; start of address is identified by
    ; di -> offset in memory_master_map
    ; ch -> first free bit in ds:di
    .found:
        xchg bx, bx
        push di
        push cx
            ; Write 1 to memory map for reserved bytes
            mov ax, [cs:.requested_bytes]
            mov bl, ch ; Save start bit
            mov cx, MEMORY_SMALLEST_UNIT
            xor dx, dx
            div cx
            cmp dx, 0
            je .found_write_no_remainder
            inc ax ; if there is a remainder, one more 1 needs to be written, to cover the requested area

            ; ax contains the number of 1 to write
            .found_write_no_remainder:
            mov cx, ax
            .found_write_loop
                mov bh, [ds:di]
                or bh, bl ; set 1 at bit active in bl
                mov [es:di], bh ; write 1 to memory map
                shr bl, 1
                cmp bl, 0
                jne .found_write_loop_skip_inc_di
                ; Last bit of current byte reached
                inc di
                mov bl, 0b10000000
                .found_write_loop_skip_inc_di:
            loop .found_write_loop
        pop cx
        pop di

        sub di, memory_master_map
        push cx
            mov cx, 65536 / MEMORY_SMALLEST_UNIT / 8 ; size for one segment
            mov bx, 0 ; -> segment

            ; Calculate segment numer into ax
            xor dx, dx
            xchg bx, bx
            mov ax, di
            div cx
            mov bx, ax

            ; Subtract segments from dx -> offset remains
            mov ax, cx
            mul bx
            mov dx, di
            sub dx, ax

            push dx
                ; Write segment into bx
                mov ax, bx
                mov cx, 0x1000
                mul cx
                mov bx, ax
            pop dx

            ; Calculate offset (part 1)
            xchg bx, bx
            mov cx, MEMORY_SMALLEST_UNIT * 8
            mov ax, dx
            mul cx
            mov dx, ax
        pop cx

        ; Calculate offset (part 2 - add MEMORY_SMALLEST_UNIT for every set bit -> ch)
        .found_offset_part2:
            shl ch, 1
            cmp ch, 0
            je .done
            add dx, MEMORY_SMALLEST_UNIT
            jmp .found_offset_part2

    .done:
        pop es
        pop ds
        pop di
        pop si
        pop cx
        pop ax
        iret

    .not_enough_memory:
        mov bx, 0xffff
        mov dx, 0xffff
        jmp .done

    .requested_bytes dw 0
;-------------------------------------------------------------------------------
