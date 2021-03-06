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
    times 0x0200 db 0xff ; Kernel ->  64kB ( 64kB / 16B / 8b) | 0x00000 - 0x0ffff
    times 0x0200 db 0xff ; Stack  ->  64kB ( 64kB / 16B / 8b) | 0x10000 - 0x1ffff
    times 0x0c00 db 0x00 ; Free   -> 384kB (384kB / 16B / 8b) | 0x20000 - 0x7ffff
    ; End of conventional memory

    ; Extended memory
    times 0x01f8 db 0x00 ; Free   ->  63kB ( 63kB / 16B / 8b) | 0x80000 - 0x8fbff
    times 0x0200 db 0xff ; V-Buff ->  64kB ( 64kB / 16B / 8b) | 0x8fc00 - 0x9fbff
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
; Initializes all memory
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
memory_initialize_all:
    push es
    push di
    push cx
    push ax

    mov ax, 0x2000

    .loop:
        mov es, ax

        xor di, di

        mov cx, 0x8000
        .loop_init_segment:
            mov [es:di], word 0
            add di, 2
        loop .loop_init_segment

        cmp ax, 0x9000
        jae .loop

    .init_to_9fbff:
        mov ax, 0x9000
        mov es, ax

        xor di, di

        mov cx, 0xfbff
        .loop_incomplete_segment:
            mov [es:di], byte 0
            inc di
        loop .loop_incomplete_segment

    pop ax
    pop cx
    pop di
    pop es
    ret
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
memory_allocate:
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
            .found_write_loop:
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

            ; Calculate segment number into ax
            xor dx, dx
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
        cmp bx, 0xffff
        jz .no_init
        cmp dx, 0xffff
        jz .no_init

        ; s: segment address o: offset address
        ; BX:DX = ssss
        ;       +  oooo
        ;       = BX:DX = 2,5 byte address
        ; segment is always a multiple of 0x1000 (see calculation above) and thus 
        ; determines most signifikant nibble (nibble 0) of the address
        ;
        ; offset determines all other nibbles (nibble 1 to nibble 4) of the address
        ; => This could cause problems, if a hole segment is allocated, because the
        ;    returned offset could roll over
        ; To solve this problem let the segment determine nibbles 0 through 3 
        ; and let the offset determine nibble 4
        ;
        ; With a smallest memory unit of 16 bytes this means that the offset should
        ; always be zero

        push dx
            shr dx, 4  ; ignore last nibble
            add bx, dx ; adjust offset nibbles 1 through 3
        pop dx
        and dx, 0x000f ; nibbles 0 through 2 of the offset are now addressed by the segment

        ; Initialize allocated memory
        push dx
            ; Check if requested bytes are at chunk size
            xor dx, dx
            mov ax, [cs:.requested_bytes]
            mov cx, MEMORY_SMALLEST_UNIT
            div cx

            ; check if divisible without remainder
            cmp dx, 0
            jnz .alter_cx
                ; divisible without remainder, number of bytes to init equals requested bytes
                xor cx, cx
                jmp .cx_ok
            .alter_cx:
                ; cx contains MEMORY_SMALLEST_UNIT
                ; dx contains rest 
                ; intialization of whole memory chunk is required, not only of the requested
                ; bytes -> calculate rest to fill memory chunk
                sub cx, dx ; -> Rest to fill the chunk
            .cx_ok:
            add cx, word [cs:.requested_bytes] ; Number of bytes to init
        pop dx

        mov es, bx
        mov di, dx
        cld
        mov al, 0
        .init_allocated:
            stosb
        loop .init_allocated

        .no_init:

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

;-------------------------------------------------------------------------------
; Free memory
;
; input: 
;   AX -> Number of bytes to free
;   BX -> Segment to free
;   DX -> Offset to free
;
; output:
;   none
;-------------------------------------------------------------------------------
memory_free:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    pusha
    pushf
    push ds
    push es

    push cs
    pop es

    test dx, 0b1111 ; If one of these bytes is set (lsb of offset is 1 - 15), the memory was not allocated by the memory manager and can thus not be freed
    jnz .done ; Do not free anything

    cmp ax, 0
    jz .done

    mov di, memory_master_map

    push ax
        ; Calculate segment number
        push dx
            mov cx, 0x1000
            mov ax, bx
            xor dx, dx
            div cx
            mov bx, ax ; Number of segment
            mov cx, dx ; Save remainder from segment 
        pop dx

        shl cx, 4 ; segment remainder (offset) needs to be multiplied with 16 because of the division above
        add dx, cx ; add segment remainder to offset 
        jnc .no_increase_segment
        ; On carry increase segment count -> offset wrapped around
        inc bx
        .no_increase_segment:

        push dx
            mov ax, bx
            mov cx, 65536 / MEMORY_SMALLEST_UNIT / 8 ; size for one segment in memory map
            mul cx ; Calculate segmet size in memory map
            add di, ax ; Select segment in memory map
        pop dx

        ; Calculate offset (part 1)
        mov cx, MEMORY_SMALLEST_UNIT * 8
        mov ax, dx
        xor dx, dx
        div cx
        add di, ax

        ; Calculate offset (part 2 -> dx contains remainder -> identifies the start bit)
        mov ax, dx
        mov cx, MEMORY_SMALLEST_UNIT
        xor dx, dx
        div cx
        cmp dx, 0
        je .no_increase_offset_2
        xchg bx, bx ; Should not occur!
        int 0x00
        inc ax ; If there is a remainder, the full 16 byte unit was allocated -> needs to be freed
        .no_increase_offset_2:

        ; Set start bit
        mov bl, 0b10000000
        mov cl, al
        shr bl, cl
    pop ax

    mov cx, MEMORY_SMALLEST_UNIT
    xor dx, dx
    div cx
    cmp dx, 0
    je .no_increase_no_of_zero
    inc ax ; If there is a remainder, the full 16 byte unit was allocated -> needs to be freed 
    .no_increase_no_of_zero:

    mov cx, ax
    .write_loop:
        mov bh, [es:di]
        not bl ; negate bl -> we want to set a zero, not a 1
        and bh, bl ; set 0 at bit active in bl
        mov [es:di], bh ; write 0 to memory map
        not bl ; reset bl so that a 1 indicates the current bit
        shr bl, 1
        cmp bl, 0
        jne .write_loop_skip_inc_di
        ; Last bit of current byte reached
        inc di
        mov bl, 0b10000000
        .write_loop_skip_inc_di:
    loop .write_loop

    .done:
        pop es
        pop ds
        popf
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
;    iret/ret
;-------------------------------------------------------------------------------
