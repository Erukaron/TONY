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

; Root dir constants
FAT_RD_ENTRY_LENGTH         equ 32
FAT_RD_NAME_OFFSET          equ 0x00
FAT_RD_EXT_OFFSET           equ 0x08
FAT_RD_ATTRIB_OFFSET        equ 0x0b
    FAT_RD_ATTRIB_READ_ONLY_OFFSET      equ 0x01
    FAT_RD_ATTRIB_HIDDEN_OFFSET         equ 0x02
    FAT_RD_ATTRIB_SYSTEM_OFFSET         equ 0x04
    FAT_RD_ATTRIB_VOLUME_LBL_OFFSET     equ 0x08
    FAT_RD_ATTRIB_SUB_DIR_OFFSET        equ 0x10
    FAT_RD_ATTRIB_ARCHIVE_OFFSET        equ 0x20
    FAT_RD_ATTRIB_DEVICE_OFFSET         equ 0x40
    FAT_RD_ATTRIB_UNUSED                equ 0x80
FAT_RD_CLUSTER_OFFSET       equ 0x1a
FAT_RD_SIZE_OFFSET          equ 0x1c


FAT_FILE_HANDLE_OFFSET_CURSOR               equ 0x0b
FAT_FILE_HANDLE_OFFSET_FILE_SIZE_CLUSTER    equ 0x0f
FAT_FILE_HANDLE_OFFSET_SEGMENT              equ 0x11
FAT_FILE_HANDLE_OFFSET_OFFSET               equ 0x13
FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES      equ 0x15
FAT_FILE_HANDLE_OFFSET_FLAGS                equ 0x16

MAX_ROOT_ENTRIES            equ 224

%include "COMMON.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variable
;-------------------------------------------------------------------------------
fat_ident_buffer times FAT_IDENT_LENGTH + 1 db 0 ; NNNNNNNNEEE0

fat_data_start_sector dw 0 ; Start of data on disk -> Offset is boot sector and other reserved sectors (including root dir and fat)
fat_root_dir_start_sector dw 0 ; Start of root dir in disk -> Offset is boot sector and other reserved sectors (including fat)
fat_load_data_offset dw 0 ; Used to load data from disk -> Set to either fat_data_start_sector or fat_root_dir_start_sector depending on the purpose

fat_print_dot_on_disk_access db 1, 0 ; word, so it can be pushed to the stack

fat_current_drive db 0
fat_current_dir times FAT_IDENT_LENGTH db ASCII_SP ; NNNNNNNNNNN
                                       db 0        ;            0

; 0x00 (11 Bytes) Directory name in FAT format
; 0x0b (2 Bytes)  Next entry segment
; 0x0d (2 Bytes)  Next entry offset
; 0x0f (1 Byte)   unused
FAT_DIR_CHAIN_ENTRY_SIZE            equ 16
FAT_DIR_CHAIN_NAME_OFFSET           equ 0
FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET   equ 0x0b
FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET    equ 0x0d
fat_dir_chain_segment dw 0
fat_dir_chain_offset  dw 0

fat_dir_current_ident   db ".", 0
fat_dir_previous_ident  db "..", 0
FAT_SUB_DIR_SEPARATOR               equ "\"

NOT_IMPLEMENTED db "THIS FEATURE IS NOT YET IMPLEMENTED! BREAKPOINT IS GENERATED JUST BEFOR THIS CALL -> CHECK CALLER VIA STACK", 0

%include "OEMPARAM.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Initializes the directory chain
;
; input:
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_init:
    pusha

    mov ax, FAT_DIR_CHAIN_ENTRY_SIZE
    int 0xb8 ; allocate memory

    mov [cs:fat_dir_chain_segment], bx
    mov [cs:fat_dir_chain_offset], dx

    ; fill root entry
    push ds
    push es
        push cs
        pop ds
        mov es, bx
        mov si, fat_current_dir
        mov di, dx
        mov cx, FAT_IDENT_LENGTH
        rep movsb
    pop es
    pop ds

    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Loads the current/root directory table to ROOT_SEGMENT:ROOT_OFFSET
;
; input:
;   fat_current_dir -> 0 or SPACE for root directory, else directory to load
;
; output:
;   none
;-------------------------------------------------------------------------------
fat_load_directory:
    cmp byte [cs:fat_current_dir], ASCII_NUL
    jz fat_load_root_directory
    cmp byte [cs:fat_current_dir], ASCII_SP
    jz fat_load_root_directory
    
    pusha
    push ds
        xor ax, ax
        push cs
        pop ds
        mov si, fat_current_dir
        int 0xc8 ; check if dir exists -> start cluster in dx

        ; dx is start cluster
        call fat_load_file_allocation_table
        xor ax, ax ; start cluster
        mov cx, 0xffff ; read complete file
        push ds
        pop es ; es:si points to directory/file name
        push word ROOT_SEGMENT
        pop bx
        mov bp, ROOT_OFFSET ; bx:bp points to destination

        cmp dx, 0
        jz .rootdir_request
            ; default directory table request
            call fat_inc_load_file
            jmp .inc_load_done
        .rootdir_request:
            call fat_inc_load_root_dir
        .inc_load_done:
    pop ds
    popa

    ret
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
        mov word [cs:fat_root_dir_start_sector], ax

        ; Do not print '.' for rootdir or fat
        push word [cs:fat_print_dot_on_disk_access]
            mov byte [cs:fat_print_dot_on_disk_access], 0

            ; Read root dir
            push word ROOT_SEGMENT
            pop es
            mov bx, ROOT_OFFSET
            mov dl, byte [cs:fat_current_drive]
            call fat_read_device
        pop word [cs:fat_print_dot_on_disk_access]

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

        ; Do not print '.' for rootdir or fat
        push word [cs:fat_print_dot_on_disk_access]
            mov byte [cs:fat_print_dot_on_disk_access], 0

            ; Read fat into memory 
            push word FAT_SEGMENT
            pop es
            mov bx, FAT_OFFSET
            call fat_read_device
        pop word [cs:fat_print_dot_on_disk_access]
    
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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

        push cx
        push ax
            ; convert string case
            mov ah, 0 ; to upper
            mov cx, 0xffff
            int 0xd7
        pop ax
        pop cx
    pop bx ; segment to load file to 
    mov bp, di ; offset to load to

    call fat_load_directory
    mov ax, 0
    mov cx, 0xffff
    call fat_inc_load_file

    ; if printing dots on disk access, print lf after loading finished, to start in a clean line
    cmp [cs:fat_print_dot_on_disk_access], byte 0
    jz .done_skip_lf
        mov al, ASCII_LF
        int 0x90 ; putch
    .done_skip_lf:
    
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
fat_inc_load_root_dir:
    push ax
        mov ax, [cs:fat_root_dir_start_sector]
        mov [cs:fat_load_data_offset], ax
    pop ax
    jmp fat_inc_load_file_go
fat_inc_load_file:
    push ax
        mov ax, [cs:fat_data_start_sector]
        mov [cs:fat_load_data_offset], ax
    pop ax
    jmp fat_inc_load_file_go
fat_inc_load_file_go:
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
        mov dx, word [es:di + FAT_RD_CLUSTER_OFFSET] ; es:di points to start of root dir entry of the file (+26 for starting cluster)
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
                mov ax, [cs:.cluster] 
                call calculate_next_cluster
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
        ; Input for int 0xe1 is dx...
        ;mov ax, word [cs:.cluster]
        mov dx, word [cs:.cluster]
        int 0xe1 ; convert cluster to lba
        add dx, [cs:fat_load_data_offset] ; offset
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
        mov ax, [cs:.cluster] 
        call calculate_next_cluster
        mov word [cs:.cluster], dx ; next cluster
        cmp dx, 0x0ff0 ; end of file marker
        jb .load_file_stage2
        jmp .no_error

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
; Calculates the next cluster
;
; input:
;   ax -> current cluster
;
; output:
;   dx -> next cluster
;   ax -> cluster passed via stack
;   es:bx -> index for file in fat
;-------------------------------------------------------------------------------
calculate_next_cluster:
    push ax
        mov ax, FAT_SEGMENT
        mov es, ax
        mov bx, FAT_OFFSET
    pop ax

    push cx
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
    push di

    .next_sector:
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

            pop di
            stc
            ret

    .done:
        int 0xcf ; Feedback disk operation

        pop cx
        pop bx
        pop ax
        add bx, word [cs:bpbBytesPerSector] ; next sector
        inc ax ; read next sector
        loop .next_sector ; repeat until cx sectors are read
        pop di
        clc
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Writes to the device specified
;
; input: 
;   dl -> device
;   es:bx -> Buffer to write from
;   ax -> Start sector (LBA)
;   cx -> Number of sectors
;
; output:
;   carry -> On error
;-------------------------------------------------------------------------------
fat_write_device:
    push di

    .next_sector: 
        mov di, FAT_READ_RETRIES ; retries for error

        .try_write:
            push ax
            push bx
            push cx

            push bx
                mov bl, dl ; remember device
                mov dx, ax

                int 0xe2 ; Convert lba to chs

                mov dl, bl
            pop bx

            mov ah, 0x03 ; Write device es:bx
            mov al, 0x01 ; Number of sectors to write
            int 0x13
            jnc .done ; test if succeeded

            xor ax, ax ; Reset disk
            int 0x13
            dec di ; decrement error counter
            pop cx
            pop bx
            pop ax
            jnz .try_write ; attempt to read again

            pop di
            stc
            ret

    .done:
        int 0xcf ; Feedback disk operation

        pop cx
        pop bx
        pop ax
        add bx, word [cs:bpbBytesPerSector] ; next sector
        inc ax ; read next sector
        loop .next_sector ; repeat until cx sectors are read
        pop di
        clc 
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
;    dx:bx -> file size in bytes (-1 if non existant)
;-------------------------------------------------------------------------------
fat_get_file_size:
    sti ; Allow hardware interrupts to occur while processing software interrupts

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

    push cx
    push ax
        ; convert string case
        mov ah, 0 ; to upper
        mov cx, 0xffff
        int 0xd7
    pop ax
    pop cx

    call fat_load_directory
    
    call fat_search_file
    cmp ax, -1
    jne .extract_size
    mov [cs:.file_size_sectors], ax ; Set error code
    jmp .done

    .extract_size:
        push word ROOT_SEGMENT
        pop es
        mov bx, word [es:di + 0x001c] ; es:di points to start of root dir entry of the file (+28 for file size) ; Low word
        mov dx, word [es:di + 0x001e] ; second half of file size in bytes (+30) ; High word
        mov [cs:.file_size_bytes], dx
        mov [cs:.file_size_bytes + 2], bx

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
        mov dx, [cs:.file_size_bytes]
        mov bx, [cs:.file_size_bytes + 2]
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
; 0x0b (2 Bytes) Current cursor position in file (current sector)
; 0x0d (2 Bytes) Current cursor position in file (current byte in sector)
; 0x0f (2 Bytes) Current file size in clusters (updated on each write -> FAT clusters)
; 0x11 (2 Bytes) Segment of loaded file (where in memory is the file located?)
; 0x13 (2 Bytes) Offset of loaded file (where in memory is the file located?)
; 0x15 (4 Bytes) Current file size in bytes (updated each write -> bytes of raw data on the drive)
; 0x19 (1 Bytes) File flags 
;                Bit 0: 0 -> File already exists
;                       1 -> File created upon open command
; 0x1a (6 Bytes) Unused
;-------------------------------------------------------------------------------
fat_open:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push ax
    push cx
    push dx
    push di
    push es
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
        push cx
        push ax
            ; convert string case
            mov ah, 0 ; to upper
            mov cx, 0xffff
            int 0xd7
        pop ax
        pop cx
        push word [cs:.handle_segment]
        pop es
        push word [cs:.handle_offset]
        pop di
        cld 
        mov cx, FAT_IDENT_LENGTH
        rep movsb

        ; set current cursor position to 0 (sector and in sector position)
        mov ax, 0
        stosw
        stosw

    ; Get filename and store it again
    pop si
    pop ds
    push ds
    push si

    push bx ; save address offset

    int 0xc1 ; Get file size in clusters
    cmp ax, -1
    jne .file_exists 
        ; Current file size in cluster is 1 
        mov ax, 1
        stosw

        push es
        push di
            ; Write byte size to file handle
            push word [cs:.handle_segment]
            pop es
            push word [cs:.handle_offset]
            pop di

            mov word [es:di + FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES], 0 ; new file
            mov word [es:di + FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES + 2], 0 ; -> 0 bytes

            or byte [es:di + FAT_FILE_HANDLE_OFFSET_FLAGS], 0b1 ; set new file flag
        pop di
        pop es

        ; 1) Find free cluster in FAT
        push es
        push bx
            mov ax, 0 ; starting cluster
            .search_free_cluster:
                call calculate_next_cluster
                cmp dx, 0
                je .free_cluster_found
                inc ax
                jmp .search_free_cluster

            ; 2) Block cluster in FAT (setting to FFF)
            .free_cluster_found:
                mov dx, 0xfff
                test ax, 1
                jz .free_cluster_done ; even cluster, dx is already set correctly

                .free_cluster_odd:
                    shl dx, 4 ; take 12 msb 

                .free_cluster_done:
                    or [es:bx], dx
        pop bx
        pop es

        ; ax contains file starting cluster
        ; 3) Find next free ROOT DIR entry
        push es
        push di
            push ROOT_SEGMENT
            pop es
            mov di, ROOT_OFFSET

            push cx
            mov cx, word [cs:bpbRootEntries]

            .create_new_loop_root_dir:
                cmp byte [es:di + FAT_RD_NAME_OFFSET], 0
                jz .root_offset_found

                add di, FAT_RD_ENTRY_LENGTH
                loop .create_new_loop_root_dir
                jmp .root_dir_space_error

            .root_dir_space_error:
                mov bp, -1
                pop cx
                pop di
                pop es
                pop bx
                jmp .done

            .root_offset_found:
                pop cx

                ; ax contains file starting cluster
                ; ds:si contains filename
                ; 4) es:di = root dir entry -> Write filename, flags, size of 0 and FAT cluster to it
    
                ; get filename in fat format
                push es
                push di
                    push cs
                    pop es
                    mov di, fat_ident_buffer
                    call fat_transform_filename_fat_format
                pop di
                pop es

                ; load filename in fat format to ds:si
                push ds
                push si
                push di
                    push cs
                    pop ds
                    mov si, fat_ident_buffer

                    ; write it to root dir
                    push cx
                        mov cx, FAT_IDENT_LENGTH
                        cld
                        rep movsb
                    pop cx
                pop di
                pop si
                pop ds

                ; write flags
                push dx
                    xor dx, dx
                    mov [es:di + FAT_RD_ATTRIB_OFFSET], dl
                pop dx

                ; write starting cluster
                mov [es:di + FAT_RD_CLUSTER_OFFSET], ax

                ; write size (should be zero, but to be sure...)
                mov word [es:di + FAT_RD_SIZE_OFFSET], 0
                mov word [es:di + FAT_RD_SIZE_OFFSET + 2], 0

                push ax
                push bx
                push cx
                push dx
                push es
                    ; Rewrite ROOT DIR
                    ; Calculate size of root directory (cx)
                    xor cx, cx
                    xor dx, dx
                    mov ax, FAT_RD_ENTRY_LENGTH ; Every entry is 32 bytes long
                    mul word [cs:bpbRootEntries] ; Multiply by the number of root entries
                    div word [cs:bpbBytesPerSector] ; Get the size of the root directory in sectors
                    xchg ax, cx ; Store in cx

                    ; Calculate location of root directory (ax)
                    mov al, byte [cs:bpbNumberOfFATs]
                    mul word [cs:bpbSectorsPerFAT]
                    add ax, word [cs:bpbReservedSectors] ; Add bootsector and other reserved sectors

                    mov dl, [cs:fat_current_drive] ; drive

                    ; write buffer
                    push word ROOT_SEGMENT
                    pop es
                    mov bx, ROOT_OFFSET

                    call fat_write_device
                    jc .critical_write_error ; error on write

                    ; Rewrite FAT 
                    ; Calculate fat size
                    xor ax, ax
                    ;mov al, byte [bpbNumberOfFATs]
                    ; Lets just write one FAT, as the second one is redundant
                    mov al, 1
                    mul word [cs:bpbSectorsPerFAT]
                    mov cx, ax

                    ; Calculate fat location
                    mov ax, word [cs:bpbReservedSectors]

                    mov dl, [cs:fat_current_drive] ; drive

                    ; write buffer
                    push word FAT_SEGMENT
                    pop es
                    mov bx, FAT_OFFSET

                    call fat_write_device
                    jc .critical_write_error ; error on write
                pop es
                pop dx
                pop cx
                pop bx
                pop ax
        pop di
        pop es

    jmp .allocate_file_address
    .file_exists:
        ; if file exists, write file size to file handle
        stosw
        
        push es
        push di
            ; Write byte size to file handle
            push word [cs:.handle_segment]
            pop es
            push word [cs:.handle_offset]
            pop di

            mov [es:di + FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES], dx
            mov [es:di + FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES + 2], bx

            and byte [es:di + FAT_FILE_HANDLE_OFFSET_FLAGS], 0b11111110 ; set existing file flag -> Set zero to bit 0
        pop di
        pop es

    .allocate_file_address:
        pop bx ; address offset ; see above ont 0xc1
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
                call fat_load_directory
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

    .critical_write_error:
        push cs
        pop ds
        mov si, .critical_write_error_msg
        mov al, 4 ; critical
        int 0x97
        cli
        hlt
        .hang_write_error:
            nop
            jmp .hang_write_error

    .done:
        pop si
        pop ds
        pop es
        pop di
        pop dx
        pop cx
        pop ax

        iret

    .handle_segment dw 0
    .handle_offset dw 0
    .critical_write_error_msg db "Error while writing root dir or fat to disk in kernel function fat_open! Halting system!", 0
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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
;   dh -> Error code
;       0x00: no error
;       0x01: end of file reached
;       0x02: error while reading next sector
;       0x80: unknown error
;-------------------------------------------------------------------------------
fat_read_next_byte:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    ; Check if next byte is inside the size of the file
    mov dh, 0x80

    push es
    push ax
    push cx
    push si
    push ds
        push bp
        pop es

        push word [es:bx + FAT_FILE_HANDLE_OFFSET_SEGMENT]
        pop ds
        push word [es:bx + FAT_FILE_HANDLE_OFFSET_OFFSET]
        pop si

        .check_eof:
            mov [cs:.eof_reached_flag], byte 0 ; 0 = eof reached, 1 = eof not reached
            pusha
                mov ax, [es:bx + FAT_FILE_HANDLE_OFFSET_CURSOR] ; sector
                mov cx, [cs:bpbBytesPerSector]
                mul cx
                add ax, [es:bx + FAT_FILE_HANDLE_OFFSET_CURSOR + 2] ; offset in sector

                cmp [es:bx + FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES], dx ; upper 16 bit
                jb .eof_reached
                cmp [es:bx + FAT_FILE_HANDLE_OFFSET_FILE_SIZE_BYTES + 2], ax ; lower 16 bit
                jb .eof_reached

                mov [cs:.eof_reached_flag], byte 1 ; 0 = eof reached, 1 = eof not reached
                
                .eof_reached: ; [cs:.eof_reached_flag] is 0, if eof is reached
            popa

            cmp [cs:.eof_reached_flag], byte 1
            je .no_eof
                mov dh, 0x01 ; set eof error code
                mov dl, 0
                jmp .done

            .no_eof:

        ; Next byte pointer is at sector dx, byte offset ax
        mov dx, [es:bx + FAT_FILE_HANDLE_OFFSET_CURSOR]
        mov ax, [es:bx + FAT_FILE_HANDLE_OFFSET_CURSOR + 2]

        cmp [cs:bpbBytesPerSector], ax
        ja .sector_ok
            ; Load next sector into memory
            inc dx

            ; Read file to address
            ; Get filename and store it again
            push es
            push ds
            pusha
                call fat_load_directory
                ;   ax -> Start sector 
                ;   cx -> Sector count (0xffff to read the complete file)
                ;   es:si -> Filename to load
                ;   bx:bp -> Buffer to load file to
                ;
                ; output:
                ;   carry -> on error
                ;   cx -> Number of loaded sectors
                mov ax, dx
                mov cx, 1
                ; First 11 bytes of handler are file name in 83 notation
                ; File buffer is at ds:si needs to be at bx:bp
                ; File name is at bp:bx needs to be at es:si
                push ds
                    push si
                        push bp
                            push bx
                            pop si
                        pop es
                    pop bp
                pop bx
                call fat_inc_load_file
            popa
            pop ds
            pop es
            jc .error_reading_file

            mov ax, 0 ; Zero byte offset for current sector

        .sector_ok:
            add si, ax
            add ax, 1 ; new offset in sector

        .update_cursor:
            mov [es:bx + FAT_FILE_HANDLE_OFFSET_CURSOR], dx ; (new) sector
            mov [es:bx + FAT_FILE_HANDLE_OFFSET_CURSOR + 2], ax ; new offset in sector

        .read_byte:
            mov dl, [ds:si]
            mov dh, 0 ; No error
            jmp .done

        .error_reading_file:
            mov dh, 0x02 ; error while reading next sector
            jmp .done

        .done:
            pop ds
            pop si
            pop cx
            pop ax
            pop es
            iret

    .eof_reached_flag db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Read next string 
;
; input: 
;   bp:bx -> Address of file handle
;   es:di -> Address to write the bytes to (zero terminated)
;   ah -> Function
;       0: read string to zero termination
;       1: read string to line break (crlf not included!)
;   cx -> Max string length
;
; output:
;   es:di -> Zero terminated string
;   cx -> Zero, if no space left
;   dh -> Error code
;       0x00: no error
;       0x01: end of file reached
;       0xff: Function (AX) not supported!
;-------------------------------------------------------------------------------
fat_read_next_string:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push di
    push ax

    mov dh, 0xff

    cmp ah, 1
    ja .done ; Option not supported

    mov dh, 0

    dec cx ; remove one iteration for zero termination

    .build_str:
        int 0xc4

        cmp dh, EOF
        jz .done

        mov al, dl
        stosb

        cmp ah, 0
        jz .check_zero

        cmp ah, 1
        jz .check_crlf

        loop .build_str
    jmp .done

    .check_zero:
        cmp [es:di - 1], byte 0
        jz .check_zero_found
        loop .build_str

        .check_zero_found:
            dec di ; di is terminated with zero in .done -> "overwrite" existing zero termination, do not terminate two times
            jmp .done

    .check_crlf:
        cmp di, 2 ; there need to be at least 2 bytes
        jb .build_str

        cmp [es:di - 2], word ASCII_CRLF
        jz .check_crlf_found
        cmp [es:di - 2], word ASCII_LFCR
        jz .check_crlf_found
        loop .build_str

        .check_crlf_found:
            dec di 
            mov byte [es:di], 0 ; terminate first CR/LF
            dec di ; di is terminated with zero in .done -> "overwrite" existing zero termination, do not terminate two times
                   ; This will terminate the second LF/CR
            jmp .done

    .done:
        ; Zero termination
        mov al, 0
        stosb 

        pop ax
        pop di

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
;   dl -> Error code
;       0x00: no error
;-------------------------------------------------------------------------------
fat_write_next_byte:
    ; Writing process:
    ; 1) Check if file exists (Create flag in file handle, set upon open)
    ;   1a) file exists: Get first cluster from root directory
    ;   1b) file does not exist:
    ;       1b.1) Check root dir for enough space (exit if not)
    ;       1b.2) Check fat for enough space (exit if not)
    ;       1b.3) Get first free cluster in fat
    ;       1b.4) Write file information from file handle to root directory (name and flags)
    ; 2) Current file pointer at sector bound
    ;   2a) yes: 
    ;       2a.1) Check if enough space (exit if not)
    ;       2a.2) Zero size of one sector in memory
    ;       2a.3) Link new cluster to old cluster in fat
    ;       2a.4) Terminate file in fat
    ;       2a.5) Write fat
    ;   2b) no: Read sector including current byte position into memory
    ; 3) Add dl to file pointer in memory
    ; 4) Increase file size in file handle
    ; 5) Write sector in memory to disk
    ; 6) Write file information from file handle to root directory (size)

    sti ; Allow hardware interrupts to occur while processing software interrupts

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
    sti ; Allow hardware interrupts to occur while processing software interrupts

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
    sti ; Allow hardware interrupts to occur while processing software interrupts

    cmp [cs:fat_print_dot_on_disk_access], byte 0
    jz .done

    mov al, FAT_DISK_OPERATION_IDENT
    int 0x90
    push ax
        xor ah, ah
        int 0x9b ; flip buffer
    pop ax

    .done:
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Directory exists
;
;   input:
;       ah -> 0
;       ds:si -> directory name
;
;   output:
;       al -> 1 directory does exist
;       dx -> start cluster of directory (-1 if non existant)
;-------------------------------------------------------------------------------
fat_directory_exists:
    sti ; Allow hardware interrupts to occur while processing software interrupts
    push ds
    push si
    push es
    push di
    push bx
    push cx
    push bp
    push ax
        cld

        mov cx, FAT_IDENT_LENGTH
        mov ah, 0 ; convert to upper
        int 0xd7 ; convert string case ds:si

        push word ROOT_SEGMENT
        pop es
        mov di, ROOT_OFFSET

        ; search for directory
        mov cx, MAX_ROOT_ENTRIES
        .serach_directories:
            mov bp, di ; entry start
            mov bx, si

            cmp byte [es:di], 0
            jz .does_not_exist ; no more entries

            mov al, FAT_RD_ATTRIB_SUB_DIR_OFFSET
            test [es:di + FAT_RD_ATTRIB_OFFSET], al
            jz .next_entry

            ; String equality check in ds:si and es:di
            push cx
                mov cx, FAT_IDENT_LENGTH
                repz cmpsb
            pop cx
            jz .does_exist ; equality for all bytes -> dir exists
            cmp byte [es:di - 1], ASCII_SP ; ASCII_SP means the directory name is not the full 11 bytes long
            jnz .next_entry ; if equality check fails and the directory name is not checked on each char, this means there is a difference
            cmp byte [ds:si - 1], ASCII_NUL
            jz .does_exist ; if however, the directory name is checked to the last char and the string in ds:si is also at its end, the names are equal

            .next_entry:
                mov di, bp
                mov si, bx
                add di, FAT_RD_ENTRY_LENGTH
        loop .serach_directories
        .directories_done:

    .does_not_exist:
        pop ax
        mov al, 0
        mov dx, -1
        jmp .done
    .does_exist:
        pop ax
        mov al, 1
        mov di, bp
        mov dx, [es:di + FAT_RD_CLUSTER_OFFSET] ; start cluster of the current entry
        jmp .done
    .done:
        ; When adding pop's see .does_exist and .does_not_exist!
        pop bp
        pop cx
        pop bx
        pop di
        pop es
        pop si
        pop ds
        iret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Directory load interrupt
;
;   input:
;       ah -> 0
;       ds:si -> directory name
;
;   output:
;       none
;-------------------------------------------------------------------------------
fat_directory_load:
    sti ; Allow hardware interrupts to occur while processing software interrupts
    pusha
    push ds
    push es
        mov byte [cs:.previous_requested], FALSE

        push cs
        pop es

        mov ah, 0b01 ; string equals case sensitive
        mov di, fat_dir_current_ident
        int 0xd2 ; check if current dir is requested
        jnc .done ; directory already ok -> do nothing

        mov ah, 0b01 ; string equals case sensitive
        mov di, fat_dir_previous_ident
        int 0xd2 ; check if previous dir is requested
        jc .load_dir
            mov byte [cs:.previous_requested], TRUE
        .load_dir:

        mov di, fat_current_dir
        mov al, ASCII_SP
        mov cx, FAT_IDENT_LENGTH + 1
        int 0xd1 ; initialize string es:di

        int 0xd0 ; get string length ds:si

        xor bx, bx
        mov dx, ax ; length of string
        int 0xd3 ; create copy of string
        add di, dx
        mov byte [es:di], ASCII_SP ; remove zero termination
        sub di, dx

        push ds
            push es
            pop ds
            mov si, fat_current_dir
            mov ah, 0
            mov cx, FAT_IDENT_LENGTH
            int 0xd7 ; String case to upper
        pop ds

        call fat_load_directory

        cmp byte [cs:.previous_requested], TRUE
        jz .previous
            call .add_entry_to_chain
        jmp .done

    .previous:
        call .remove_last_entry_from_chain
        jmp .done

    ; adds the directory in fat_current_dir to the directory chain
    .add_entry_to_chain:
        cld

        push word [cs:fat_dir_chain_segment]
        pop es
        mov di, [cs:fat_dir_chain_offset]

        push cs
        pop ds
        mov si, fat_current_dir

        call .load_last_entry

        ; Create and append new entry 
        mov ax, FAT_DIR_CHAIN_ENTRY_SIZE
        int 0xb8
        mov [es:di + FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET], bx
        mov [es:di + FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET], dx

        ; Load new entry
        mov es, bx
        mov di, dx 

        ; Copy name to entry
        mov cx, FAT_IDENT_LENGTH
        rep movsb

        ret

    ; removes the last entry from the directory chain
    .remove_last_entry_from_chain:
        push word [cs:fat_dir_chain_segment]
        pop es
        mov di, [cs:fat_dir_chain_offset]

        call .load_last_entry

        ; check if current entry is last entry
        cmp bx, 0
        jnz .remove_last_entry
        cmp dx, 0 
        jnz .remove_last_entry

        ; current is last -> do not remove
        ret

        .remove_last_entry:
            ; deallocate the memory of the last element
            push bx
                mov ax, FAT_DIR_CHAIN_ENTRY_SIZE
                mov bx, es
                mov dx, di
                int 0xb9
            pop bx

            ; unlink the last element from the chain
            ; setting the seg/off of the last element in the second last element to zero
            push es
                mov es, bx
                mov word [es:bp + FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET], 0
                mov word [es:bp + FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET], 0
            pop es

            ret

    ; loads the last entry in the dir chain addressed by es:di
    ; returns last entry in es:di 
    ; returns second last entry in bx:bp (if any, bx:bp = 0 if only 1 entry)
    .load_last_entry:
        mov word [cs:.load_last_entry_previous_segment], 0
        mov word [cs:.load_last_entry_previous_offset], 0

        ; search last entry
        .last_search:
            mov bx, [es:di + FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET]
            mov bp, [es:di + FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET]
            cmp bx, 0
            jnz .load_next
            cmp bp, 0
            jnz .load_next

            jmp .found_last

            .load_next:
                mov [cs:.load_last_entry_previous_segment], es
                mov [cs:.load_last_entry_previous_offset], di
                mov es, bx
                mov di, bp
                jmp .last_search
        .found_last:
            mov bx, [cs:.load_last_entry_previous_segment]
            mov bp, [cs:.load_last_entry_previous_offset]
            ret
        .load_last_entry_previous_segment dw 0
        .load_last_entry_previous_offset  dw 0

    .done:
        pop ds
        pop es
        popa
        iret

    .previous_requested db FALSE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Get current directory
;
; input: 
;   ah -> 0
;
; output:
;   bx:bp -> offset to string
;-------------------------------------------------------------------------------
fat_get_current_dir:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push ds
    push es
    pusha
        ; deallocate old string
        mov bx, [cs:.current_dir_str_segment]
        mov dx, [cs:.current_dir_str_offset]
        mov ax, [cs:.current_dir_string_length]
        int 0xb9

        ; Get entry count 
        push word [cs:fat_dir_chain_segment]
        pop es
        mov di, [cs:fat_dir_chain_offset]
        call .count_entries_dir_chain

        ; Calculate required string size
        ; Each entry is 11 bytes (name) + 1 byte ("\") long
        ; String needs to be zero terminated -> + 1
        mov ax, FAT_IDENT_LENGTH + 1
        mul cl
        add ax, 1 ; zero termination
        mov [cs:.current_dir_string_length], ax

        ; Allocate string
        int 0xb8
        mov [cs:.current_dir_str_segment], bx
        mov [cs:.current_dir_str_offset], dx

        ; Load root of dir chain
        push word [cs:fat_dir_chain_segment]
        pop ds
        mov si, [cs:fat_dir_chain_offset]

        mov es, bx
        mov di, dx

        cld
        .build_string:
            call .entry_to_string
            mov al, FAT_SUB_DIR_SEPARATOR
            stosb

            cmp word [ds:si + FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET], 0
            jnz .next_entry
            cmp word [ds:si + FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET], 0
            jnz .next_entry

            ; seg and off are 0 -> last entry
            jmp .string_built

            .next_entry:
                push word [ds:si + FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET]
                push word [ds:si + FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET]
                pop si
                pop ds
                jmp .build_string

        .string_built:

        ; terminate string
        mov byte [es:di], 0
    popa
    pop es
    pop ds

    mov bx, [cs:.current_dir_str_segment]
    mov bp, [cs:.current_dir_str_offset]

    iret

    ; counts the entries in the directory chain loaded to es:di
    ; returns count in cx
    .count_entries_dir_chain:
        ; search last entry
        push bx
        push bp
        push es
        push di
        xor cx, cx
        .count_search:
            inc cx
            mov bx, [es:di + FAT_DIR_CHAIN_NEXT_SEGMENT_OFFSET]
            mov bp, [es:di + FAT_DIR_CHAIN_NEXT_OFFSET_OFFSET]
            cmp bx, 0
            jnz .count_next
            cmp bp, 0
            jnz .count_next

            jmp .count_last

            .count_next:
                mov es, bx
                mov di, bp
                jmp .count_search
        .count_last:
            pop di
            pop es
            pop bp
            pop bx
            ret

    ; Appends the directory name of the current entry (ds:si) to the string (es:di)
    ; es and di are not preserved!
    .entry_to_string:
        push ds
        push si
            push bx
            push dx
            push es
            push di
                push cs
                pop es
                mov di, .current_dir_buffer
                xor bx, bx
                mov dx, FAT_IDENT_LENGTH
                int 0xd3 ; copy string from entry in ds:si into buffer
            pop di
            pop es
            pop dx
            pop bx

            ; replace spaces in buffer with 0
            push cs
            pop ds
            mov si, .current_dir_buffer
            mov dl, ASCII_SP
            mov dh, ASCII_NUL
            mov cx, FAT_IDENT_LENGTH
            int 0xd6 ; replace spaces with nul

            ; get length of string in buffer
            int 0xd0 ; length in ax (terminator excluded)

            ; write buffer to string (es:di)
            cld
            mov cx, ax
            rep movsb
        pop si
        pop ds
        ret

    .current_dir_string_length  dw 0
    .current_dir_str_segment    dw 0
    .current_dir_str_offset     dw 0
    .current_dir_buffer         times FAT_IDENT_LENGTH  db ASCII_SP
                                                        db 0
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