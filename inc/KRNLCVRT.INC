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
; TONY Kernel convert module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
%include "OEMPARAM.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Converts a string to upper/lower case
;
; input:
;   ah -> 0 convert to upper, not 0 convert to lower
;   ds:si -> string to convert
;
; output:
;   ds:si -> Converted string
;-------------------------------------------------------------------------------
convert_string_case:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push si

    .loop:
        cmp byte [ds:si], 0
        je .done

        cmp ah, 0 ; check mode
        jne .convert_lower

        ; Convert higher
        ; Check if character is a lower character and skip if not
        cmp byte [ds:si], ASCII_A_LOWER
        jl .next
        cmp byte [ds:si], ASCII_Z_LOWER
        jg .next

        sub byte [ds:si], ASCII_A_LOWER - ASCII_A ; subract offset 
        jmp .next

    .convert_lower:
        ; Check if character is a upper character and skip if not
        cmp byte [ds:si], ASCII_A
        jl .next
        cmp byte [ds:si], ASCII_Z
        jg .next

        add byte [ds:si], ASCII_A_LOWER - ASCII_A ; add offset 

    .next:
        inc si
        jmp .loop

    .done:
        pop si
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Convert Cluster to LBA (Logical Block Address)
;
; input: 
;   dx -> Cluster to convert
;
; output:
;   dx -> (cluster - 2) * sectors per cluster (LBA)
;-------------------------------------------------------------------------------
convert_cluster_lba:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push cx
    push ax

    sub dx, 2 ; Cluster number needs to be zero based
    xor cx, cx
    mov cl, byte [cs:bpbSectorsPerCluster] 
    mov ax, dx
    mul cx
    ;add ax, word [cs:_data_start_sector] 
    mov dx, ax

    pop ax
    pop cx
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Convert LBA to CHS (Cylinder Head Sector)
;
; input: 
;   dx -> LBA Address to convert
;
; output:
;   cl -> Sector: (logical sector / sectors per track) + 1
;   dh -> Head: (logical sector / sectors per track) MOD number of heads
;   ch -> Track: logical sector / (sectors per track * number of heads)
;-------------------------------------------------------------------------------
convert_lba_chs:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    mov ax, dx
    xor dx, dx ; prepare dx:ax for operation
    div word [cs:bpbSectorsPerTrack] 
    inc dl ; adjust for sector 0
    mov byte [cs:.lba_chs_absolute_sector], dl

    xor dx, dx ; prepare dx:ax for operation
    div word [cs:bpbHeadsPerCylinder]
    mov byte [cs:.lba_chs_absolute_head], dl
    mov byte [cs:.lba_chs_absolute_track], al

    mov cl, [cs:.lba_chs_absolute_sector]
    mov dh, [cs:.lba_chs_absolute_head]
    mov ch, [cs:.lba_chs_absolute_track]

    iret

    .lba_chs_absolute_sector db 0
    .lba_chs_absolute_head db 0
    .lba_chs_absolute_track db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Took straight from the Snowdrop OS project (file: string.asm) 
; written by Sebastian Mihai, http://sebastianmihai.com
; from which "the source code is fully available, without any restrictions on 
; its usage and modification."
;
; Converts a 16-bit string representation of a decimal unsigned integer to an 
; unsigned integer.
;
; input:
;   DS:SI -> pointer to string representation of integer (zero-terminated)
;
; output:
;   AX -> resulting integer
;-------------------------------------------------------------------------------
convert_unsigned_16bit_atoi:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push bx
    push cx
    push dx
    push si
    push di
    
    mov ax, 0                               ; 0 is default for bad values
    cmp byte [ds:si], 0
    je .string_unsigned_16bit_atoi_exit      ; NOOP when empty string
    
    mov cx, 0                               ; CX will hold intermediate results
    mov bx, 1                               ; multiplier
    
    mov di, si
.string_unsigned_16bit_atoi_go_to_units:
    cmp byte [ds:di+1], 0                       ; is DS:DI on the units digit?
    je .string_unsigned_16bit_atoi_accumulate    ; yes
    inc di                                      ; no, move to the right
    jmp .string_unsigned_16bit_atoi_go_to_units
.string_unsigned_16bit_atoi_accumulate:
    ; DS:DI now points to the units digit
    mov al, byte [ds:di]                    ; AL := ASCII of digit
    sub al, ASCII_0                         ; AL := digit value
    mov ah, 0                               ; AX := digit value
    mul bx                                  ; DX:AX := digit * multiplier
    ; ASSUMPTION: DX=0, since the string contains a 16bit unsigned integers
    add ax, cx                              ; AX := intermediate result
    
    cmp si, di                              ; have we just accumulated the
                                            ; most significant digit?
    je .string_unsigned_16bit_atoi_exit      ; yes, so we're done
    dec di                                  ; no, move to the left
    
    mov cx, ax                              ; CX := intermediate result
    
    mov ax, bx                              ; AX := multiplier
    mov bx, 10
    mul bx                                  ; DX:AX := multiplier * 10
    ; ASSUMPTION: DX=0, since the string contains a 16bit unsigned integers
    mov bx, ax                              ; BX := multiplier * 10
    jmp .string_unsigned_16bit_atoi_accumulate   ; process this digit
.string_unsigned_16bit_atoi_exit:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Converts a number into printable hex format
;
; input:
;   DL -> Number to transform to printable hex
;
; output:
;   DX -> resulting printable ascii integer
;-------------------------------------------------------------------------------
convert_bin_ascii_hex:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    mov dh, dl
    shr dh, 4 ; set high nibble of dl to low nibble of dh
    and dl, 0x0f ; delete high nibble of dl

    cmp dl, 9
    jg .dl_letter
    add dl, 0x30 ; Offset for 0
    jmp .dl_done
    .dl_letter:
    add dl, 'A' ; Offset for A
    sub dl, 10 ; Offset for numerics
    .dl_done:

    cmp dh, 9
    jg .dh_letter
    add dh, 0x30 ; Offset for 0
    jmp .dh_done
    .dh_letter:
    add dh, 'A' ; Offset for A
    sub dh, 10 ; Offset for numerics
    .dh_done:

    .done:
        iret
;-----------------------------------------------------------------------------

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