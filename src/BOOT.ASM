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
; TONY Bootloader
;-------------------------------------------------------------------------------
bits 16
org 0
jmp start
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Include files
;--------------------------------------------------------------------------------------
%include "OEMPARAM.INC"
%include "COMMON.INC"
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Loader
;--------------------------------------------------------------------------------------
start:
    cli ; Disable interrups while segments are set up

    ; copy bootloader to 0x40000
    push BOOT_SEGMENT
    pop ds
    mov si, BOOT_OFFSET
    mov di, BOOT_COPY_OFFSET
    push word BOOT_COPY_SEGMENT
    pop es
    mov cx, 512
    cld
    rep movsb
    jmp BOOT_COPY_SEGMENT:boot_loader_ready

boot_loader_ready:
    mov ax, BOOT_COPY_SEGMENT
    mov ds, ax
    xor si, si
    xor di, di

    mov ax, STACK_SEGMENT
    mov ss, ax
    xor sp, sp

    push dx ; dl contains boot-drive

    sti

    mov si, boot_msg
    call print

    call load_root_directory
    jc halt

    mov si, kernel_filename
    call load_file
    jc halt

    mov si, crlf
    call print

    pop dx ; Get bootdrive in dl

    jmp KERNEL_SEGMENT:KERNEL_OFFSET
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Loads the root directory to 7c00:0200 (carry on error)
;--------------------------------------------------------------------------------------
load_root_directory:
    ; Calculate size of root directory (cx)
    xor cx, cx
    xor dx, dx
    mov ax, 0x0020 ; Every entry is 32 bytes long
    mul word [bpbRootEntries] ; Multiply by the number of root entries
    div word [bpbBytesPerSector] ; Get the size of the root directory in sectors
    xchg ax, cx ; Store in cx

    ; Calculate location of root directory (ax)
    mov al, byte [bpbNumberOfFATs]
    mul word [bpbSectorsPerFAT]
    add ax, word [bpbReservedSectors] ; Add bootsector and other reserved sectors

    ; Calculate start of data sector
    mov word [data_start_sector], ax
    add word [data_start_sector], cx

    ; Read root dir
    mov bx, 0x0200
    call read_device

    ret
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Searches for the file in ds:si (Carry on error)
;--------------------------------------------------------------------------------------
load_file:
    mov cx, [bpbRootEntries] ; Max entry count
    mov di, 0x0200 ; First byte of root directory

    .find_file:
        push cx

        mov cx, 11 ; Name is 11 characters
        push si
        push di
        rep cmpsb ; Tests if ds:si and es:di are equal and sets the zero flag accordingly (rep stops on zero flag = 0)

        pop di ; Reset di to start of filename
        pop si ; Reset si to start of filename
        pop cx
        je .load_fat

        add di, 32 ; Next entry
        loop .find_file
        stc ; No files left carry on error
        jmp .done

    .load_fat:
        mov dx, [di + 26] ; First cluster of file
        mov word [file_start_cluster], dx ; First cluster of the file

        ; Calculate how many sectors both of the fats use (into cx)
        xor ax, ax
        mov al, [bpbNumberOfFATs]
        mul word [bpbSectorsPerFAT]
        mov cx, ax

        ; Calculate the starting address of the fat (into ax)
        mov ax, [bpbReservedSectors]

        ; Read fat
        mov dl, [bsDriveNumber]
        mov bx, 0x0200
        call read_device

    .load_file:
        ; Read file to 0x0000:0x8000
        mov ax, KERNEL_SEGMENT
        mov es, ax
        mov bx, KERNEL_OFFSET
        push bx

        .load_file_2:
            ; Read cluster
            mov ax, word [file_start_cluster]
            pop bx

            call convert_cluster_to_lba

            xor cx, cx
            mov cl, byte [bpbSectorsPerCluster]
            call read_device
            push bx

            ; Calculate next cluster
            mov ax, word [file_start_cluster] ; current cluster 

            ; FAT12 saves cluster information in a 12 BIT fashion -> check if cluster is odd or even to only use the correct 12 from 16 bits
            mov cx, ax
            mov dx, ax
            shr dx, 1 ; divide dx by 2
            add cx, dx ; 3/2 cluster

            mov bx, 0x0200 ; ds:bx location of fat in memory
            add bx, cx ; index for fat
            mov dx, word[bx] ; get two bytes from fat

            test ax, 1
            jnz .odd_cluster

            .even_cluster:
                and dx, 0000111111111111b ; take 12 lsb
                jmp .cluster_arithmethic_done

            .odd_cluster:
                shr dx, 4 ; take 12 msb

            .cluster_arithmethic_done:
                mov word [file_start_cluster], dx ; new start cluster for file
                cmp dx, 0x0ff0 ; test for end of file
                jb .load_file_2
                pop bx

    .done:
        ret
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Reads the device specified by dl to es:bx for cx sectors starting from ax (carry on error)
;--------------------------------------------------------------------------------------
read_device:
    mov di, 0x0005 ; five retries for error
    mov byte [.drive_no], dl

    .try_load:
        push ax
        push bx
        push cx

        call convert_lba_to_chs ; convert starting sector to CHS

        mov ah, 0x02 ; Read device es:bx
        mov al, 0x01 ; Number of sectors to read
        mov ch, byte [absoluteTrack] ; track
        mov cl, byte [absoluteSector] ; sector
        mov dh, byte [absoluteHead] ; head
        mov dl, byte [.drive_no]
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
        pop cx
        pop bx
        pop ax
        ret

    .done:
        mov si, progress_msg
        call print
        pop cx
        pop bx
        pop ax
        add bx, word [bpbBytesPerSector] ; next sector
        inc ax ; read next sector
        loop read_device ; repeat until cx sectors are read
        ret

    .drive_no db 0
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Convert Cluster to LBA (Logical Block Address)
; ax=>Cluster to convert
; LBA = (cluster - 2) * sectors per cluster
;--------------------------------------------------------------------------------------
convert_cluster_to_lba:
    sub ax, 2 ; Cluster number needs to be zero based
    xor cx, cx
    mov cl, byte [bpbSectorsPerCluster] 
    mul cx
    add ax, word [data_start_sector] ; base data sector
    ret
;--------------------------------------------------------------------------------------
     
;--------------------------------------------------------------------------------------
; Convert LBA to CHS
; ax=>LBA Address to convert
;
; absolute sector = (logical sector / sectors per track) + 1
; absolute head   = (logical sector / sectors per track) MOD number of heads
; absolute track  = logical sector / (sectors per track * number of heads)
;--------------------------------------------------------------------------------------
convert_lba_to_chs:
    xor     dx, dx ; prepare dx:ax for operation
    div     word [bpbSectorsPerTrack] 
    inc     dl ; adjust for sector 0
    mov     byte [absoluteSector], dl

    xor     dx, dx ; prepare dx:ax for operation
    div     word [bpbHeadsPerCylinder]
    mov     byte [absoluteHead], dl
    mov     byte [absoluteTrack], al

    ret
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Prints the null terminated string in DS:SI
;--------------------------------------------------------------------------------------
print:
    pusha 

    .exec:
        lodsb
        cmp al, 0
        je .done

        mov ah, 0x0e
        int 0x10

        jmp .exec

    .done:
        popa
        ret
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Halts the system on fatal error
;--------------------------------------------------------------------------------------
halt:
    mov si, halt_msg
    call print 

    int 0x18 ; No bootable device...
    cli
    hlt
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Data
;--------------------------------------------------------------------------------------
boot_msg db "TONY: BOOT", 0
halt_msg db "ERROR", 0

progress_msg db ".", 0
crlf db 0x0a, 0x0d, 0

kernel_filename db "KERNEL  SYS"

data_start_sector dw 0
file_start_cluster dw 0

; LBA/CHS convert
absoluteSector db 0
absoluteHead   db 0
absoluteTrack  db 0
;--------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------
; Boot-Magic
;--------------------------------------------------------------------------------------
times 510-($-$$) nop
db 0x55
db 0xaa
;---------------------------------