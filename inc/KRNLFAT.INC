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
FAT_FILE_HANDLE_SIZE        equ 32

FAT_FILE_HANDLE_OFFSET_CURSOR       equ 0x0b
FAT_FILE_HANDLE_OFFSET_FILE_SIZE    equ 0x0f
FAT_FILE_HANDLE_OFFSET_SEGMENT      equ 0x11
FAT_FILE_HANDLE_OFFSET_OFFSET       equ 0x13

FAT_ROOTDIR_OFFSET_START_CLUSTER    equ 0x1a

%include "COMMON.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variable
;-------------------------------------------------------------------------------
fat_ident_buffer times FAT_IDENT_LENGTH + 1 db 0 ; NNNNNNNNEEE0
fat_data_start_sector dw 0 ; Start of data on disk -> Offset is boot sector and other reserved sectors
fat_print_dot_on_disk_access db 1
fat_current_drive db 0
NOT_IMPLEMENTED db "THIS FEATURE IS NOT YET IMPLEMENTED! BREAKPOINT IS GENERATED JUST BEFOR THIS CALL -> CHECK CALLER VIA STACK", 0

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
    mov ax, 0
    mov cx, 0xffff
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
;   ax -> Start cluster 
;   cx -> Sector count (0xffff to read the complete file)
;   es:si -> Filename to load
;   bx:bp -> Buffer to load file to
;
; output:
;   carry -> on error
;   cx -> Number of loaded sectors
;-------------------------------------------------------------------------------
fat_inc_load_file:
    mov word [cs:.loaded_sectors], 0

    .find:
        push bx
        push bp
            push es
            pop ds
            push ax
                call fat_search_file
                cmp ax, -1
                jne .load_file_stage1
        ; Else exit
            pop ax
        pop bp
        pop bx
        stc
        ret

    .load_file_stage1:
        pop ax ; Start cluster
        ; Calculate starting cluster ES:DI is set to First byte of entry in root dir
        mov dx, word [es:di + FAT_ROOTDIR_OFFSET_START_CLUSTER] ; es:di points to start of root dir entry of the file (+26 for starting cluster)
        mov word [cs:.cluster], dx

        ; target location (bx:bp -> es:bx)
        pop bx ; See push in .find
        pop es
        push bx
        push es

        call fat_load_file_allocation_table

        ; Advance to start cluster (ax)
        .advance_to_start_cluster:
            cmp ax, 0
            je .start_cluster_ok

            push ax
                call .calculate_next_cluster
                mov word [cs:.cluster], dx ; next cluster
                cmp dx, 0x0ff0 ; end of file marker
                jb .advance_to_start_cluster_no_eof
                    ; Error -> Start cluster is larger than file
                    pop ax
                    pop es
                    pop bx
                    stc 
                    ret

                .advance_to_start_cluster_no_eof:
            pop ax
            dec ax
            jmp .advance_to_start_cluster
        .start_cluster_ok:

    .load_file_stage2:
        cmp cx, 0
        je .no_error ; max sector count 

        ; Read the file into memory
        mov ax, word [cs:.cluster]
        int 0xe1 ; convert cluster to lba
        add dx, [cs:fat_data_start_sector] ; offset
        mov ax, dx
        mov dl, [cs:fat_current_drive]

        ; target location
        pop es
        pop bx

        push cx
            xor cx, cx
            mov cl, byte [cs:bpbSectorsPerCluster]
            call fat_read_device
        pop cx

        ; Increase sectors read
        inc word [cs:.loaded_sectors]
        dec cx

        ; Save for next iteration
        push bx
        push es

        ; Get the next cluster from fat
        call .calculate_next_cluster
        mov word [cs:.cluster], dx ; next cluster
        cmp dx, 0x0ff0 ; end of file marker
        jb .load_file_stage2
        jmp .no_error

    .calculate_next_cluster:
        mov ax, FAT_SEGMENT
        mov es, ax
        mov bx, FAT_OFFSET

        push cx
            mov ax, word [cs:.cluster] ; current cluster
            mov cx, ax
            mov dx, ax
            shr dx, 1 ; divide by 2
            add cx, dx ; sum for 3/2 cluster

            add bx, cx ; index for file in fat
            mov dx, word [es:bx] ; read two bytes (16 bit (but we only need 12) from fat)
        pop cx
        test ax, 1
        jnz .odd_cluster

        .even_cluster:
            and dx, 0000111111111111b ; take 12 lsb
            jmp .done_cluster_arithmetic

        .odd_cluster:
            shr dx, 4 ; take 12 msb 
            jmp .done_cluster_arithmetic

        .done_cluster_arithmetic:
            ret

    .no_error:
        clc ; No error 
        jmp .done

    .done:
        pop es
        pop bx
        ;pop cx
        mov cx, [cs:.loaded_sectors]
        ret

    .cluster dw 0
    .loaded_sectors dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Searches for a filename in the Rootdirectory 
;
; input: 
;   ds:si -> Filename
;
; output:
;   ax -> File index in directory table (-1 if error)
;   es:di -> First byte of entry if file was found
;-------------------------------------------------------------------------------
fat_search_file:
    mov ax, ROOT_SEGMENT ; di is later set to root_offset, thus es needs to be zero
    mov es, ax

    push dx
    push cx
    push bx

    mov bx, si ; Save filename

    ; Browse root directory for file...
    mov cx, word [cs:bpbRootEntries]
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
;   ax -> Start sector (LBA)
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
        add bx, word [cs:bpbBytesPerSector] ; next sector
        inc ax ; read next sector
        loop fat_read_device ; repeat until cx sectors are read
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Get file size
;
; input:
;    ds:si -> filename in 8.3 format
;
; output:
;    ax -> file size in sectors (-1, if file does not exist)
;-------------------------------------------------------------------------------
fat_get_file_size:
    pusha
    push ds
    push es

    ; Extract filename -> Write to cs:fat_ident_buffer
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
    pop es

    call fat_load_root_directory
    
    call fat_search_file
    cmp ax, -1
    jne .extract_size
    mov [cs:.file_size_sectors], ax ; Set error code
    jmp .done

    .extract_size:
        push word ROOT_SEGMENT
        pop es
        mov dx, word [es:di + 0x001c] ; es:di points to start of root dir entry of the file (+28 for file size) ; Low word
        mov bx, word [es:di + 0x001e] ; second half of file size in bytes (+30) ; High word
        mov [cs:.file_size_bytes], bx
        mov [cs:.file_size_bytes + 2], dx

    ; Divide File size by bytes per sector
    ; Max file size: max(AX)*bytes_per_sector = 0xffff*512 = 31GiB
    mov dx, [cs:.file_size_bytes]
    mov ax, [cs:.file_size_bytes + 2]
    mov cx, [cs:bpbBytesPerSector]
    div cx
    cmp dx, 0
    je .no_inc_sector
    inc ax
    .no_inc_sector:
    mov [cs:.file_size_sectors], ax

    ; see: http://tbruylan.blogspot.com/2013/01/32-bit-division-on-old-school-8086.html
    ;push es
    ;    push cs
    ;    push cs
    ;    pop ds
    ;    pop es
    ;    cld
        
    ;    mov si, .file_size_bytes
    ;    mov di, .file_size_sectors
    ;    mov cx, [cs:bpbBytesPerSector]

;        xor dx, dx ; set DX := 0
;        lodsw ; load the higher 16-bits into AX
;        div cx ; divide DX:AX by CX
;               ; leaves DX with remainder and AX with quotient
;        stosw ; write out AX to result

    ;    lodsw ; load the lower 16-bits into AX
    ;    div cx ; divide DX::AX by CX
    ;           ; leaves DX with remainder and AX with quotient
    ;    stosw ; write out AX to result
    ;pop es

    .done:
        pop es
        pop ds
        popa
        mov ax, [cs:.file_size_sectors]
        iret

    .file_size_bytes dd 0 ; 4 byte file size
    .file_size_sectors dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Open (or create) file
;
; input: 
;   ds:si -> filename
;
; output:
;   bp:bx -> Address of file handle (bp=-1 if not enough memory)
;-------------------------------------------------------------------------------
; File handle:
; 0x00 (11 Bytes) File name in fat format
; 0x0b (4 Bytes) Current cursor position in file
; 0x0f (2 Bytes) Current file size in clusters (updated on each write)
; 0x11 (2 Bytes) Segment of loaded file
; 0x13 (2 Bytes) Offset of loaded file
; 0x15 (9 Bytes) Unused
;-------------------------------------------------------------------------------
fat_open:
    push ds
    push si

        mov ax, FAT_FILE_HANDLE_SIZE
        push bx
            int 0xb8 ; Allocate file handle
            cmp dx, 0xffff
            jne .save_handle_address
            mov bp, -1
            pop bx
            jmp .done

            .save_handle_address:
            mov [cs:.handle_segment], bx
            mov [cs:.handle_offset], dx     
        pop bx

        ; Extract filename
        ; es is set to target location for file, set to current segment to access strings
        push es
            push cs
            pop es
            push di
                mov di, fat_ident_buffer
                call fat_transform_filename_fat_format
            pop di
        pop es

        ; Write filename to handle
        push cs
        pop ds
        mov si, fat_ident_buffer
        push word [cs:.handle_segment]
        pop es
        push word [cs:.handle_offset]
        pop di
        cld 
        mov cx, FAT_IDENT_LENGTH
        rep movsb

        ; set cursor position to 0
        mov ax, 0
        stosw
        stosw

    ; Get filename and store it again
    pop si
    pop ds
    push ds
    push si
    int 0xc1 ; Get file size
    cmp ax, -1
    jne .file_exists 
        ; Current file size in cluster is 1 
        mov ax, 1
        stosw
        ; Create file in fat and root dir
    jmp .not_impl
    jmp .allocate_file_address
    .file_exists:
        ; if file exists, write file size to file handle
        stosw

    .allocate_file_address:
        mov ax, [cs:bpbBytesPerSector]
        int 0xb8 ; Allocate one sector
        cmp dx, 0xffff
        jne .write_file_address_to_handle
            ; Not enough space to allocate file
        jmp .error_deallocate_handle
        .write_file_address_to_handle:
            mov ax, bx
            stosw
            mov ax, dx
            stosw

            ; Read file to address
            ; Get filename and store it again
            push es
            pusha
                call fat_load_root_directory
                ;   ax -> Start sector 
                ;   cx -> Sector count (0xffff to read the complete file)
                ;   es:si -> Filename to load
                ;   bx:bp -> Buffer to load file to
                ;
                ; output:
                ;   carry -> on error
                ;   cx -> Number of loaded sectors
                mov ax, 0
                mov cx, 1
                ; First 11 bytes of handler are file name in 83 notation
                push word [cs:.handle_segment]
                pop es
                push word [cs:.handle_offset]
                pop si
                ;mov si, si
                ;mov bx, bx
                mov bp, dx
                call fat_inc_load_file
            popa
            pop es
            jc .error_deallocate_file_and_handle

            ; Set bp:bx to file handle location and exit
            push word [cs:.handle_segment]
            pop bp
            push word [cs:.handle_offset]
            pop bx
        jmp .done

    .error_deallocate_file_and_handle:
        ; Deallocate file
        mov ax, [cs:.handle_segment]
        mov ds, ax
        mov si, [cs:.handle_offset]
        mov ax, [cs:bpbBytesPerSector]
        mov bx, [ds:si + FAT_FILE_HANDLE_OFFSET_SEGMENT]
        mov dx, [ds:si + FAT_FILE_HANDLE_OFFSET_OFFSET]
        int 0xb9
    .error_deallocate_handle:
        ; Deallocate filehandle and return
        mov ax, FAT_FILE_HANDLE_SIZE
        mov bx, [cs:.handle_segment]
        mov dx, [cs:.handle_offset]
        int 0xb9
        mov bp, -1 ; error code
    jmp .done

    .done:
        pop si
        pop ds
        iret

    .not_impl:
        xchg bx, bx
        push cs
        pop ds
        mov si, NOT_IMPLEMENTED
        int 0x91
        cli 
        hlt
        .hang:
        nop
        jmp .hang

    .handle_segment dw 0
    .handle_offset dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Close file
;
; input: 
;   bp:bx -> Address of file handle
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_close:
    push ds
    pusha

    push bx
        ; Deallocate file
        mov ds, bp
        mov si, bx
        mov ax, [cs:bpbBytesPerSector]
        mov bx, [ds:si + FAT_FILE_HANDLE_OFFSET_SEGMENT]
        mov dx, [ds:si + FAT_FILE_HANDLE_OFFSET_OFFSET]
        int 0xb9
    pop bx

    ; Deallocate filehandle
    mov ax, FAT_FILE_HANDLE_SIZE
    mov dx, bx
    mov bx, bp
    int 0xb9

    popa
    pop ds
    iret 
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Read next byte
;
; input: 
;   bp:bx -> Address of file handle
;
; output:
;   dl -> Read byte
;-------------------------------------------------------------------------------
fat_read_next_byte:
    push ds
    push si
    push ax
    push dx

    xchg bx, bx
    push cs
    pop ds
    mov si, NOT_IMPLEMENTED
    int 0x91
    cli 
    hlt
    .hang:
    nop
    jmp .hang

    ; Check if next byte is inside the size of the file

    ;push word [bp:bx + FAT_FILE_HANDLE_OFFSET_SEGMENT]
    ;pop ds
    ;push word [bp:bx + FAT_FILE_HANDLE_OFFSET_OFFSET]
    ;pop si

    ;mov ax, [bp:bx + FAT_FILE_HANDLE_OFFSET_CURSOR]
    ;mov dx, [bp:bx + FAT_FILE_HANDLE_OFFSET_CURSOR + 2]
    ; Next byte is at ax:dx

    ; Check if loaded sector contains file byte offset ax:dx
        ; If not, load required sector
    ; Read byte into dl

    ; Update cursor
    inc dx
    jnc .no_inc_32_bit
    inc ax
    .no_inc_32_bit:

    ;mov [bp:bx + FAT_FILE_HANDLE_OFFSET_CURSOR], ax
    ;mov [bp:bx + FAT_FILE_HANDLE_OFFSET_CURSOR + 2], dx

    pop dx
    pop ax
    pop si
    pop ds

    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Read next bytes
;
; input: 
;   bp:bx -> Address of file handle
;   cx -> Number of bytes to read
;   es:di -> Address to write the bytes to
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_read_next_bytes:
    xchg bx, bx
    push cs
    pop ds
    mov si, NOT_IMPLEMENTED
    int 0x91
    cli 
    hlt
    .hang:
    nop
    jmp .hang
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Write next byte
;
; input: 
;   bp:bx -> Address of file handle
;   dl -> Byte to write
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_write_next_byte:
    xchg bx, bx
    push cs
    pop ds
    mov si, NOT_IMPLEMENTED
    int 0x91
    cli 
    hlt
    .hang:
    nop
    jmp .hang
    iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Write next bytes
;
; input: 
;   bp:bx -> Address of file handle
;   cx -> Number of bytes to write
;   ds:si -> Address to read the bytes from
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_write_next_bytes:
    xchg bx, bx
    push cs
    pop ds
    mov si, NOT_IMPLEMENTED
    int 0x91
    cli 
    hlt
    .hang:
    nop
    jmp .hang
    iret
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