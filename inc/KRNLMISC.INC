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
; TONY Kernel machine language terminal module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
MLT_BUFFER_SIZE     equ 80
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Starts the machine language terminal 
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
mlt_start:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    pusha
    pushf
    push es
    push ds

    push cs
    pop ds
    push cs
    pop es

    .loop:
        mov al, '*'
        int 0x90

        mov cx, MLT_BUFFER_SIZE
        mov di, .mlt_buffer
        mov al, 0x00
        int 0xd1 ; initialize string

        mov cx, MLT_BUFFER_SIZE
        mov di, .mlt_split_buffer
        mov al, 0x00
        int 0xd1 ; initialize string

        mov di, .mlt_buffer
        int 0xb1 ; read line

        mov si, .mlt_buffer
        cmp word [es:si], 0
        jz .loop

    jmp .loop

    pop ds
    pop es
    popf
    popa

    .mlt_buffer       times MLT_BUFFER_SIZE db 0
    .mlt_split_buffer times MLT_BUFFER_SIZE db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Call COM program
;
; input: 
;   DS:SI -> Filename of COM program
;   ES:DI -> Command line arguments as string
;
; output:
;   AX -> Error code
;       0x00: No error
;       0x01: File not found
;       0x02: Not enough RAM
;       0x04: File to large
;-------------------------------------------------------------------------------
; COM list:
; 0x00 (3 Bytes) Segment + offset of file of the called program
; 0x03 (3 Bytes) Segment + offset of stack befor the sub program was called
; 0x06 (3 Bytes) Segment + offset of next list element (0xfffff, if end of list)
; 0x09 (1 Byte)  Flags
;                Bit 0 -> 0: There is a next entry
;                         1: This entry is the last in the list
; 0x0a (2 Bytes) Number of bytes reserved for the file (including 0x100 psp bytes)
; 0x0c (4 Bytes) Unused
MISC_COM_LIST_FILE_OFFSET       equ 0x00
MISC_COM_LIST_STACK_OFFSET      equ 0x03
MISC_COM_LIST_NEXT_OFFSET       equ 0x06
MISC_COM_LIST_FLAGS_OFFSET      equ 0x09
MISC_COM_LIST_SIZE_OFFSET       equ 0x0a
MISC_COM_LIST_ENTRY_LENGTH      equ 0x10

misc_com_list_start_segment dw 0
misc_com_list_start_offset dw 0
%include "OPCODES.INC"
;-------------------------------------------------------------------------------
misc_start_com:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    xor ax, ax

    pusha
    push ds
    push es

    int 0xc1 ; file exists if ax not -1
    cmp ax, -1
    jz .error_file_not_found
    cmp dx, 0 ; max file size: 0x0000feff 0x100 reserved for psp
    ja .error_file_to_large

    ; File will be read to sector boundary -> reserve space to next sector boundary
    xor dx, dx
    mov ax, bx
    push ax
        mov cx, [cs:bpbBytesPerSector]
        div cx ; dx is rest
        sub cx, dx
    pop ax
    add ax, cx ; add rest to fill the sector boundary

    ; dx:bx is file size in bytes
    ; allocate filesize + 0x100 for psp
    ; 32 bit addition
    xor dx, dx
    mov cx, 0
    mov bx, PSP_LENGTH
    mov bp, 0
    int 0x8e
    cmp bp, 1 ; on overflow
    jz .error_file_to_large

    ; Allocate memory
    int 0xb8
    cmp dx, 0xffff
    jz .error_not_enough_ram

    mov [cs:.com_segment], bx
    mov [cs:.com_offset], dx

    ; Load file
    push es
    push di
        mov es, bx
        mov di, dx
        add di, PSP_LENGTH ; save first 0x100 byte for psp
        int 0xc0 ; load file to es:di
    pop di
    pop es

    ; Allocate memory for new list element
    push ax ; Save size reserved for file
        mov ax, 16
        int 0xb8
    pop ax
    cmp bx, 0xffff ; Check if enough space for new list entry
    jz .error_not_enough_ram

    cmp word [cs:misc_com_list_start_segment], 0
    jz .first_entry

    .chain_entry:
        push ds
        push si
        push ax
            push word [cs:misc_com_list_start_segment]
            pop ds
            mov si, [cs:misc_com_list_start_offset]

            .chain_entry_loop:
                mov al, [ds:si + MISC_COM_LIST_FLAGS_OFFSET]
                test al, 1 ; Bit 0 is set for the last entry
                jnz .chain_entry_found

                ; next entry
                push word [ds:si + MISC_COM_LIST_NEXT_OFFSET]
                    mov al, [ds:si + MISC_COM_LIST_NEXT_OFFSET + 2]
                    and ax, 0x000f
                    mov si, ax 
                pop ds
            jmp .chain_entry_loop

            .chain_entry_found:
            ; New entry -> zero bit 0 in current entry
            and byte [ds:si + MISC_COM_LIST_FLAGS_OFFSET], 0xfe

            push bx
            push dx
                ; calculate absolute memory location -> first (3 nibbles of offset + 4 nibbles of segment) * 16 + last nibble of offset
                mov ax, dx
                shr ax, 4 
                add bx, ax ; segment
                and dx, 0x000f ; remove segment information

                ; Write location of next liste element to current list element
                mov word [ds:si + MISC_COM_LIST_NEXT_OFFSET], bx ; segment
                or byte [ds:si + MISC_COM_LIST_NEXT_OFFSET + 2], dl ; offset
            pop dx
            pop bx
        pop ax
        pop si
        pop ds

        jmp .write_entry

    .first_entry:
        mov [cs:misc_com_list_start_segment], bx
        mov [cs:misc_com_list_start_offset], dx

    ; current list entry is located at bx:dx
    ; ax contains file size + psp
    .write_entry:
        mov bp, sp

        push es
        push di
            push bx
            pop es
            mov di, dx

            ; Write filesize
            mov [es:di + MISC_COM_LIST_SIZE_OFFSET], ax

            ; calculate absolute memory location of file -> first (3 nibbles of offset + 4 nibbles of segment) * 16 + last nibble of offset
            mov ax, [cs:.com_offset]
            shr ax, 4 
            add [cs:.com_segment], ax 
            and word [cs:.com_offset], 0x000f ; remove segment information

            ; Write location of file to current list element
            mov ax, [cs:.com_segment]
            mov word [es:di + MISC_COM_LIST_FILE_OFFSET], ax
            mov ax, [cs:.com_offset] 
            or byte [es:di + MISC_COM_LIST_FILE_OFFSET + 2], al

            ; calculate absolute memory location of stack -> first (3 nibbles of offset + 4 nibbles of segment) * 16 + last nibble of offset
            mov ax, bp
            shr ax, 4 
            mov bx, ss
            add bx, ax
            and bp, 0x000f ; remove segment information

            ; Write location of stack to current list element
            mov word [es:di + MISC_COM_LIST_STACK_OFFSET], bx
            mov ax, bp
            or byte [es:di + MISC_COM_LIST_STACK_OFFSET + 2], al

            ; Last entry of chain, set flag
            mov byte [es:di + MISC_COM_LIST_FLAGS_OFFSET], 1

            ; Calculate file location
            mov ax, [cs:.com_segment]
            mov ds, ax
            mov si, [cs:.com_offset]
        pop di
        pop es

    ;Build psp with command line arguments -> file is loaded at ds:si + 0x100
    pusha
    push si
        ; Zero psp
        cld
        mov cx, PSP_LENGTH
        .psp_zero:
            mov byte [ds:si], 0
            inc si
            loop .psp_zero

        mov si, 0
        mov word [ds:si], OP_INT_20h ; int 0x20 op-code -> terminate program

        mov si, PSP_CMD_LINE_OFFSET 
        mov cx, PSP_CMD_LINE_MAX_LENGTH 
        .psp_build_cmd:
            cmp byte [es:di], 0
            jz .psp_build_cmd_done
            mov al, [es:di]
            mov [ds:si], al
            inc si
            inc di
            loop .psp_build_cmd

        .psp_build_cmd_done:
            mov bx, PSP_CMD_LINE_MAX_LENGTH
            sub bx, cx
            mov [ds:PSP_CMD_LINE_LENGTH_OFFSET], bl 
    pop si
    popa

    ; Set segments to com segment
    push ds
    pop es

    push word 0 ; on ret -> jump to psp entry 0 -> int 0x20 -> exit

    sti ; we want to receive interrupts in the com program

    push ds    ; segment of com file
    push PSP_LENGTH ; code offset of com file
    retf       ; give control to com file

    .error_file_not_found:
        popa
        or ax, 1
        iret

    .error_not_enough_ram:
        popa
        or ax, 2
        iret

    .error_file_to_large:
        popa
        or ax, 4
        iret

    .com_segment dw 0
    .com_offset  dw 0
;-------------------------------------------------------------------------------
; Terminates the com program that was last added to the com task list
;-------------------------------------------------------------------------------
misc_end_active_com:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp word [cs:misc_com_list_start_segment], 0
    jz .error ; call without running program

    push word [cs:misc_com_list_start_segment]
    pop ds
    mov si, word [cs:misc_com_list_start_offset]

    .find_last_entry:
        mov al, [ds:si + MISC_COM_LIST_FLAGS_OFFSET]
        test al, 1 ; Bit 0 is set for the last entry
        jnz .last_entry_found

        ; next entry
        push word [ds:si + MISC_COM_LIST_NEXT_OFFSET]
            mov al, [ds:si + MISC_COM_LIST_NEXT_OFFSET + 2]
            and ax, 0x000f
            mov si, ax 
        pop ds
    jmp .find_last_entry
    .last_entry_found:

    mov bp, [ds:si + MISC_COM_LIST_FILE_OFFSET]
    jmp misc_end_com

    .error:
        iret
;-------------------------------------------------------------------------------
; Terminates the com program, which base address (segment - 0x100 bytes psp) is located at bp
; For a program located at 0x20100 bp needs to be 0x20000
;-------------------------------------------------------------------------------
misc_end_com:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp word [cs:misc_com_list_start_segment], 0
    jz .done ; call without running program

    ; Load first list entry
    push word [cs:misc_com_list_start_segment]
    pop ds
    mov si, word [cs:misc_com_list_start_offset]

    mov byte [cs:.is_first_entry], TRUE

    ; Find entry matching to address in bp
    .find_entry:
        cmp bp, [ds:si + MISC_COM_LIST_FILE_OFFSET]
        jz .entry_found

        mov al, [ds:si + MISC_COM_LIST_FLAGS_OFFSET]
        test al, 1 ; Bit 0 is set for the last entry
        jnz .done ; Requested com process does not exist

        mov byte [cs:.is_first_entry], FALSE

        ; save previous entry
        push ds
        pop es
        mov di, si

        ; next entry
        push word [ds:si + MISC_COM_LIST_NEXT_OFFSET]
            mov al, [ds:si + MISC_COM_LIST_NEXT_OFFSET + 2]
            and ax, 0x000f
            mov si, ax 
        pop ds
    jmp .find_entry
    .entry_found:

    ; deallocate program + psp memory
    mov bx, [ds:si + MISC_COM_LIST_FILE_OFFSET]
    mov dl, [ds:si + MISC_COM_LIST_FILE_OFFSET + 2]
    and dl, 0x0f
    mov ax, [ds:si + MISC_COM_LIST_SIZE_OFFSET]
    int 0xb9

    ; remove entry from chain (previous entry is located at es:di)
    ; check if the current entry is the first entry
    cmp byte [cs:.is_first_entry], TRUE
    jz .current_is_first
        mov al, [ds:si + MISC_COM_LIST_FLAGS_OFFSET]
        test al, 1
        jnz .current_is_last
            ; link previous entry to next entry of current entry -> unchaining current entry
            mov ax, [ds:si + MISC_COM_LIST_NEXT_OFFSET]
            mov dl, [ds:si + MISC_COM_LIST_NEXT_OFFSET + 2]
            mov [es:di + MISC_COM_LIST_NEXT_OFFSET], ax
            mov [es:di + MISC_COM_LIST_NEXT_OFFSET + 2], dl
            jmp .rebuild_stack
        .current_is_last:
            ; if the current entry is the last one, just set the last entry bit in the previous entry -> unchaining current entry
            or byte [es:di + MISC_COM_LIST_FLAGS_OFFSET], 1
            jmp .rebuild_stack
    .current_is_first:
        ; Check if first is only entry
        mov al, [ds:si + MISC_COM_LIST_FLAGS_OFFSET]
        test al, 1
        jnz .current_is_only
            ; Write location of new first entry to vars
            mov ax, [ds:si + MISC_COM_LIST_NEXT_OFFSET]
            xor dx, dx
            mov dl, [ds:si + MISC_COM_LIST_NEXT_OFFSET + 2]
            mov [cs:misc_com_list_start_segment], ax
            mov [cs:misc_com_list_start_offset], dx
            jmp .rebuild_stack
        .current_is_only:
            mov word [cs:misc_com_list_start_segment], 0
            mov word [cs:misc_com_list_start_offset], 0
            jmp .rebuild_stack

    .rebuild_stack:
    ; Get the stack previous to the com call from the list entry and restore its original state
    mov bx, [ds:si + MISC_COM_LIST_STACK_OFFSET]
    mov dx, bx
    shl dx, 4 ; remove segment information
    xor ax, ax
    mov al, [ds:si + MISC_COM_LIST_STACK_OFFSET + 2]
    and al, 0x0f
    or ax, dx ; ax contains value for sp
    and bx, 0xf000 ; bx contains value for ss

    cli
    mov ss, bx
    mov sp, ax
    sti

    .deallocate_list_entry:
    mov bx, ds
    mov dx, si
    mov ax, MISC_COM_LIST_ENTRY_LENGTH
    int 0xb9

    ; pop registers that were pushed upon the call
    pop es
    pop ds
    popa

    .done:
        iret

    .is_first_entry db FALSE
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
