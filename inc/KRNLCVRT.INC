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