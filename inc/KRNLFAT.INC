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
; TONY Kernel FAT12 module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
FAT_INPUT_LENGTH            equ 12
FAT_EXTENSION_FAT_LENGTH    equ 3
FAT_IDENT_LENGTH            equ 11
FAT_FILETYPE_SEPERATOR      equ '.'
FAT_READ_RETRIES            equ 8
FAT_DISK_OPERATION_IDENT    equ '.'

%include "COMMON.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variable
;-------------------------------------------------------------------------------
fat_ident_buffer times FAT_IDENT_LENGTH + 1 db 0 ; NNNNNNNNEEE0
fat_data_start_sector dw 0 ; Start of data on disk -> Offset is boot sector and other reserved sectors
fat_print_dot_on_disk_access db 1
fat_current_drive db 0

%include "OEMPARAM.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Loads the root directory table to ROOT_SEGMENT:ROOT_OFFSET
;
; input:
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_load_root_directory:
    pusha 
    push es

    ; Calculate size of root directory (cx)
    xor cx, cx
    xor dx, dx
    mov ax, 0x0020 ; Every entry is 32 bytes long
    mul word [cs:bpbRootEntries] ; Multiply by the number of root entries
    div word [cs:bpbBytesPerSector] ; Get the size of the root directory in sectors
    xchg ax, cx ; Store in cx

    ; Calculate location of root directory (ax)
    mov al, byte [cs:bpbNumberOfFATs]
    mul word [cs:bpbSectorsPerFAT]
    add ax, word [cs:bpbReservedSectors] ; Add bootsector and other reserved sectors

    ; Calculate start of data sector
    mov word [cs:fat_data_start_sector], ax
    add word [cs:fat_data_start_sector], cx

    ; Read root dir
    push word ROOT_SEGMENT
    pop es
    mov bx, ROOT_OFFSET
    mov dl, byte [cs:fat_current_drive]
    call fat_read_device
    pop es

    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Loads the FAT to FAT_SEGMENT:FAT_OFFSET
;
; input:
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_load_file_allocation_table:
    pusha
    push es

    ; Calculate fat size
    xor ax, ax
    ;mov al, byte [bpbNumberOfFATs]
    ; Lets just read one FAT, as the second one is redundant
    mov al, 1
    mul word [cs:bpbSectorsPerFAT]
    mov cx, ax

    ; Calculate fat location
    mov ax, word [cs:bpbReservedSectors]

    ; Read fat into memory 
    push word FAT_SEGMENT
    pop es
    mov bx, FAT_OFFSET
    call fat_read_device
    pop es

    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Load file from current drive
;
; input:
;    ds:si -> filename in 8.3 format
;    es:di -> read buffer
;
; output:
;    carry flag set on error
;-------------------------------------------------------------------------------
fat_load:
    pushf
    pusha
    push ds
    push es

    ; Clear entire buffer
    cld
    ;push di
    ;.clear_buffer:
    ;    mov al, 0
    ;    stosb
    ;    cmp di, 0
    ;    jne .clear_buffer
    ;pop di

    ; Extract filename
    ; es is set to target location for file, set to current segment to access strings
    push es
    push cs
    pop es
        push di
            mov di, fat_ident_buffer
            call fat_transform_filename_fat_format
        pop di

        ; Load file specified by fat_ident_buffer
        push cs
        pop ds
        mov si, fat_ident_buffer
    pop bx ; segment to load file to 
    mov bp, di ; offset to load to

    call fat_load_root_directory
    call fat_inc_load_file
    
    pop es
    pop ds
    popa
    popf
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Transformes a 8.3 formatted filename into a fat filename
;
; input: 
;   ds:si -> filename to transform
;
; output:
;   es:di -> output buffer for fat filename (needs to be exactly 11 bytes wide, no 0 terminator required, no check)
;-------------------------------------------------------------------------------
fat_transform_filename_fat_format:
    pusha 

    push es
    push cs
    pop es
        mov al, 0
        mov di, .filename_copy
        mov cx, FAT_INPUT_LENGTH + 1
        int 0xd1

        mov al, 0
        mov di, .fileextension
        mov cx, FAT_EXTENSION_FAT_LENGTH + 1
        int 0xd1
    pop es


    mov al, ASCII_SP
    mov di, fat_ident_buffer
    mov cx, FAT_IDENT_LENGTH + 1
    int 0xd1

    mov ax, cs

    push ds
        push es
            mov es, ax

            push ax
                int 0xd0 ; get length of filename
                inc ax ; add 0 terminator
                mov dx, ax
            pop ax

            mov bx, 0
            mov di, .filename_copy
            int 0xd3 ; copy substring

            mov ds, ax
            mov ah, 10b ; zero terminate
            mov al, FAT_FILETYPE_SEPERATOR
            mov si, .filename_copy
            mov di, .fileextension
            mov cx, FAT_EXTENSION_FAT_LENGTH
            int 0xd4 ; split string
        pop es

        ; Save actual filename
        mov si, .filename_copy
        int 0xd0 ; get length of filename
        dec ax

        mov bx, 0
        mov dx, ax
        mov di, fat_ident_buffer
        int 0xd3 ; copy filename to output buffer

        ; Replace zero terminator with space
        push di
        add di, ax
        mov [cs:di], byte ASCII_SP
        pop di

        ; Save filetype
        add di, FAT_INPUT_LENGTH - FAT_EXTENSION_FAT_LENGTH - 1
        mov si, .fileextension
        int 0xd0 ; length of extension
        dec ax

        mov bx, 0
        mov dx, ax
        int 0xd3 ; copy fileexteion to output buffer

        ; Replace zero terminator with space
        cmp ax, FAT_EXTENSION_FAT_LENGTH
        jge .no_replace_terminator ; if not less, there is no terminator to be replaced!
        push di
        add di, ax
        mov [cs:di], byte ASCII_SP
        pop di

        .no_replace_terminator:

        mov ax, es
        mov ds, ax
        mov ah, 0
        mov si, di
        int 0xe0 ; Convert string to upper
    pop ds

    popa
    ret

    .filename_copy times FAT_INPUT_LENGTH + 1 db 0 ; Format: NNNNNNNN.EEE0 ; Zero terminated
    .fileextension times FAT_EXTENSION_FAT_LENGTH + 1 db 0 ; Needs to be 0 terminated -> EEE0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Loads a file 
;
; input: 
;   es:si -> Filename to load
;   bx:bp -> Buffer to load file to
;
; output:
;   carry -> on error
;   cx -> Number of loaded sectors
;-------------------------------------------------------------------------------
fat_inc_load_file:
    xor cx, cx
    push cx

    .find:
        push bx
        push bp
        push es
        pop ds
        call fat_search_file
        cmp ax, -1
        jne .load_file_stage1
        pop bp
        pop bx
        pop cx
        stc
        ret

    .load_file_stage1:
        ;sub di, ROOT_OFFSET
        ;sub ax, ROOT_OFFSET

        ; Calculate starting cluster
        push word ROOT_SEGMENT
        pop es
        mov dx, word [es:di + 0x001a] ; es:di points to start of root dir entry of the file (+26 for starting cluster)
        mov word [.cluster], dx

        ; target location (bx:bp -> es:bx)
        pop bx ; See push in .find
        pop es
        push bx
        push es

        call fat_load_file_allocation_table

    .load_file_stage2:
        ; Read the file into memory
        mov ax, word [.cluster]
        pop es
        pop bx
        int 0xe1 ; convert cluster to lba
        add dx, [cs:fat_data_start_sector] ; offset
        mov ax, dx
        mov dl, [cs:fat_current_drive]

        xor cx, cx
        mov cl, byte [bpbSectorsPerCluster]
        call fat_read_device

        ; Increase sectors read
        pop cx
        inc cx
        push cx

        ; Save for next iteration
        push bx
        push es

        ; Calculate next cluster
        mov ax, FAT_SEGMENT
        mov es, ax
        mov bx, FAT_OFFSET

        mov ax, word [.cluster] ; current cluster
        mov cx, ax
        mov dx, ax
        shr dx, 1 ; divide by 2
        add cx, dx ; sum for 3/2 cluster

        add bx, cx ; index for file in fat
        mov dx, word [es:bx] ; read two bytes (16 bit (but we only need 12) from fat)
        test ax, 1
        jnz .odd_cluster

    .even_cluster:
        and dx, 0000111111111111b ; take 12 lsb
        jmp .done_cluster_arithmetic

    .odd_cluster:
        shr dx, 4 ; take 12 msb 
        jmp .done_cluster_arithmetic

    .done_cluster_arithmetic:
        mov word [.cluster], dx ; next cluster
        cmp dx, 0x0ff0 ; end of file marker
        jb .load_file_stage2

        clc ; No error 
        jmp .done

    .done:
        pop es
        pop bx
        pop cx
        ret

    .cluster dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Searches for a filename in the Rootdirectory 
;
; input: 
;   ds:si -> Filename
;   es:di -> First byte of entry if file was found
;
; output:
;   ax -> File index in directory table (-1 if error)
;-------------------------------------------------------------------------------
fat_search_file:
    mov ax, ROOT_SEGMENT ; di is later set to root_offset, thus es needs to be zero
    mov es, ax

    push dx
    push cx
    push bx

    mov bx, si ; Save filename

    ; Browse root directory for file...
    mov cx, word [bpbRootEntries]
    mov di, ROOT_OFFSET
    cld ; Clear direction for string operation

    .exec:
        push cx
        mov cx, 11
        mov si, bx
        push di
        rep cmpsb
        pop di

        je .found

        pop cx
        add di, 32

        loop .exec

        .not_found:
            mov ax, -1
            jmp .done

        .found:
            pop ax ; old cx value -> entry number in root dir
            jmp .done

    .done:          
        pop bx
        pop cx
        pop dx

        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Reads the device specified
;
; input: 
;   dl -> device
;   es:bx -> Buffer to read to
;   ax -> Start sector
;   cx -> Number of sectors
;
; output:
;   carry -> On error
;-------------------------------------------------------------------------------
fat_read_device:
    mov di, FAT_READ_RETRIES ; retries for error

    .try_load:
        push ax
        push bx
        push cx

        push bx
        mov bl, dl ; remember device
        mov dx, ax

        int 0xe2 ; Convert lba to chs

        mov dl, bl
        pop bx

        mov ah, 0x02 ; Read device es:bx
        mov al, 0x01 ; Number of sectors to read
        int 0x13
        jnc .done ; test if succeeded

        xor ax, ax ; Reset disk
        int 0x13
        dec di ; decrement error counter
        pop cx
        pop bx
        pop ax
        jnz .try_load ; attempt to read again

        stc
        ret

    .done:
        int 0xcf ; Feedback disk operation

        pop cx
        pop bx
        pop ax
        add bx, word [bpbBytesPerSector] ; next sector
        inc ax ; read next sector
        loop fat_read_device ; repeat until cx sectors are read
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Feedback on disk operation
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_callback_disk_operation:
    cmp [cs:fat_print_dot_on_disk_access], byte 0
    je .done

    mov al, FAT_DISK_OPERATION_IDENT
    int 0x90

    .done:
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
;    iret / ret
;-------------------------------------------------------------------------------