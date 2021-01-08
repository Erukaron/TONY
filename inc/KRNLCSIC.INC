;-------------------------------------------------------------------------------
; MIT License
;
; Copyright (c) 2021 Erukaron
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
; TONY Kernel COSIC-Language module
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Constants
;-------------------------------------------------------------------------------
COSIC_LIST_LABELS_ENTRY_LENGTH       equ 16
COSIC_LIST_POINTER_ENTRY_LENGTH      equ 16
COSIC_LIST_HANDLES_ENTRY_LENGTH      equ 16
COSIC_LIST_STRVARS_ENTRY_LENGTH      equ 16
COSIC_LIST_INTVARS_ENTRY_LENGTH      equ 16

COSIC_LIST_TYPE_LABEL                equ 1
COSIC_LIST_TYPE_POINTER              equ 2
COSIC_LIST_TYPE_HANDLE               equ 4
COSIC_LIST_TYPE_STRVAR               equ 8
COSIC_LIST_TYPE_INTVAR               equ 16

COSIC_LIST_FLAG_OFFSET               equ 0x00
COSIC_LIST_NEXT_OFFSET               equ 0x01
COSIC_LIST_NAME_LENGTH_OFFSET        equ 0x04
COSIC_LIST_NAME_OFFSET               equ 0x05

COSIC_LIST_INTVARS_DIGITS_OFFSET     equ 0x08
COSIC_LIST_INTVARS_FLAG_OFFSET       equ 0x09
COSIC_LIST_INTVARS_VALUE_OFFSET      equ 0x0a

COSIC_LIST_INTVARS_FLAG_SIGN         equ 0x01

COSIC_INSTRUCTION_MAX_LENGTH         equ 255
COSIC_ARGUMENT_SEPERATOR             equ ' '

COSIC_ERROR_NO_ERROR                 equ 0x00
COSIC_ERROR_MEMORY_LIMIT             equ 0x01
COSIC_ERROR_LIST_TYPE_NOT_SUPPORTED  equ 0x02
COSIC_ERROR_INSTRUCTION_SIZE         equ 0x03
COSIC_ERROR_INSTRUCTION_NOT_IMPL     equ 0x04
COSIC_ERROR_SIZE_NEGATIVE            equ 0x05
COSIC_ERROR_SIZE_INTEGER             equ 0x06
COSIC_ERROR_UNDEFINED_SYMBOL         equ 0x07
COSIC_ERROR_EXPECTED_TO              equ 0x10
COSIC_ERROR_EXPECTED_FROM            equ 0x11
COSIC_ERROR_EXPECTED_BY              equ 0x12
COSIC_ERROR_EXPECTED_GIVING          equ 0x13
COSIC_ERROR_EXPECTED_PARAM           equ 0x14
COSIC_ERROR_EXPECTED_THEN            equ 0x15
COSIC_ERROR_EXPECTED_DO              equ 0x16
COSIC_ERROR_EXPECTED_WITH            equ 0x17
COSIC_ERROR_EXPECTED_FOR             equ 0x18
COSIC_ERROR_EXPECTED_NUMERIC         equ 0x19
COSIC_ERROR_EXPECTED_PERCENT         equ 0x1a
COSIC_ERROR_EXPECTED_NUMERIC_LITERAL equ 0x1b
COSIC_ERROR_EXPECTED_ASSIGN_OR_SIZE  equ 0x1c
;COSIC_ERROR_                         equ 0x20
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Variables
;-------------------------------------------------------------------------------
; ToDo: next_instruction_line
;       current_instruction_line
;-------------------------------------------------------------------------------
; General List structure
; 0x00 (1 Bytes) Flags
;   Bit 0: Last list element, if set
; 0x01 (3 Bytes) Next List element Pointer (word -> Segment, low nibble of byte is offset)
; 0x01 (1 Bytes) Object name length (String is not 0 terminated, because it is located inside the source code of the program which is running)
; 0x05 (3 Bytes) Object name pointer (Pointer to string of var name / label name)
; 0x08 (n Bytes) Start of specific list entry
;-------------------------------------------------------------------------------
; ToDo: instruction_line_after_label word
;       number of params byte
;       pointer to var 3 bytes
cosic_list_labels_segment  dw 0
cosic_list_labels_offset   dw 0
; ToDo: pointer 3 Bytes
cosic_list_pointer_segment dw 0
cosic_list_pointer_offset  dw 0
; ToDo: type of handle (1 byte)
;       Pointer to (e.g.) file handle
cosic_list_handles_segment dw 0
cosic_list_handles_offset  dw 0
; ToDo: str length (word)
;       string
cosic_list_strvars_segment dw 0
cosic_list_strvars_offset  dw 0
; 0x08 (1 Bytes) Number of digits reserved
; 0x09 (1 Bytes) Flags
;   Bit 0: Value is negative, if set
; 0x0a (Number of digits reserved / 2 Bytes) Stored value
;      Numbers are stored in bcd format -> each nibble represents one digit
;      Most significant nibble is most significant digit
;      -> One byte stores two digits
cosic_list_intvars_segment dw 0
cosic_list_intvars_offset  dw 0
;-------------------------------------------------------------------------------
; Insturction string
cosic_inst_comment         db ";", 0
cosic_inst_variable_int    db "%", 0
cosic_inst_variable_str    db "$", 0
cosic_inst_move            db "move", 0
cosic_inst_display         db "display", 0
cosic_inst_input           db "input", 0
cosic_inst_add             db "add", 0
cosic_inst_sub             db "subtract", 0
cosic_inst_append          db "append", 0
cosic_inst_if              db "if", 0
cosic_inst_elseif          db "elseif", 0
cosic_inst_else            db "else", 0
cosic_inst_end             db "end", 0
cosic_inst_while           db "while", 0
cosic_inst_for             db "for", 0
cosic_inst_read            db "read", 0
cosic_inst_write           db "write", 0
cosic_inst_locate          db "locate", 0
cosic_inst_assign          db "assign", 0
cosic_inst_size            db "size", 0
cosic_inst_call            db "call", 0
cosic_inst_return          db "return", 0
cosic_inst_multiply        db "multiply", 0
cosic_inst_divide          db "divide", 0
cosic_inst_waitkey         db "waitkey", 0
cosic_inst_cast            db "cast", 0
cosic_inst_delete          db "delete", 0
cosic_inst_random          db "random", 0
cosic_inst_fopen           db "fopen", 0
cosic_inst_fclose          db "fclose", 0
cosic_inst_fget            db "fget", 0
cosic_inst_fput            db "fput", 0
cosic_inst_fread           db "fread", 0
cosic_inst_fwrite          db "fwrite", 0
cosic_inst_execute         db "execute", 0
cosic_inst_exit            db "exit", 0
cosic_inst_goto            db "goto", 0
cosic_inst_literal_str     db '"', 0
cosic_inst_pointer         db "*", 0
cosic_inst_label           db ":", 0
cosic_inst_handle          db "~", 0
cosic_inst_to              db "to", 0
cosic_inst_giving          db "giving", 0
cosic_inst_from            db "from", 0
cosic_inst_by              db "by", 0
cosic_inst_param           db "param", 0
cosic_inst_accept          db "accept", 0
cosic_inst_then            db "then", 0
cosic_inst_do              db "do", 0
cosic_inst_step            db "step", 0
cosic_inst_with            db "with", 0
cosic_inst_crlf            db "crlf", 0
cosic_op_equality          db "==", 0
cosic_op_greater           db ">", 0
cosic_op_lesser            db "<", 0
cosic_op_greater_equal     db ">=", 0
cosic_op_lesser_equal      db "<=", 0
cosic_op_not               db "not", 0
cosic_op_or                db "or", 0
cosic_op_and               db "and", 0
cosic_op_xor               db "xor", 0
cosic_op_nor               db "nor", 0
cosic_op_nand              db "nand", 0
cosic_op_xnor              db "xnor", 0
cosic_op_parenthesis_open  db "(", 0
cosic_op_parenthesis_close db ")", 0
;-------------------------------------------------------------------------------
cosic_instruction_base times COSIC_INSTRUCTION_MAX_LENGTH db 0
db 0 ; zero terminator for instruction base
cosic_instruction_argument times COSIC_INSTRUCTION_MAX_LENGTH db 0
db 0 ; zero terminator for instruction argument
cosic_sourcecode_mode db FALSE ; 0 = direct mode -> variable names are cloned
                               ; 1 = sourcecode mode -> variable names are read from source code
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Execute COSIC Line
;
; input: 
;   DH -> mode
;       0: direct mode (variable names are cloned)
;       1: sourcecode mode (variable names are read from source code)
;   DS:SI -> Line of COSIC Code
;
; output:
;   AL -> Error Code
;       0: No error
;       1: Not enough ram
;       2: Instruction exceeds 255 characters 
;       4: Unknown instruction
;   AH -> Column number
;-------------------------------------------------------------------------------
cosic_execute:
    sti ; Allow hardware interrupts to occur while processing software interrupts

    push bx
    push cx
    push dx
    push bp
    push si
    push di
    push ds
    push es

    mov byte [cs:cosic_sourcecode_mode], dh

    ; ToDo: Globale Rückgabecodes
    mov byte [cs:.error_code], COSIC_ERROR_NO_ERROR

    int 0xd0 ; get string length
    cmp ax, COSIC_INSTRUCTION_MAX_LENGTH
    ja .error_instruction_size

    push ax
        mov cx, COSIC_INSTRUCTION_MAX_LENGTH
        push cs
        pop es
        mov di, cosic_instruction_base
        mov al, 0x00
        int 0xd1 ; initialize string
    pop ax

    mov di, cosic_instruction_base
    xor bx, bx
    mov dx, ax
    int 0xd3 ; copy string in ds:si to .instruction_base

    ; From now on use working copy of the string
    ; Remove Comment
    push cs
    pop ds
    mov si, cosic_instruction_base
    mov di, cosic_instruction_argument
    mov ah, 2
    mov al, byte [cs:cosic_inst_comment]
    mov cx, 0
    int 0xd4 ; Split string

    mov cx, COSIC_INSTRUCTION_MAX_LENGTH
    mov di, cosic_instruction_argument
    mov al, 0x00
    int 0xd1 ; initialize string

    ; Remove every multiple whitespace (eg double or triple spaces), so that only one remains
    mov ah, 2
    int 0xd5

    ; Get arguments
    push cs
    pop ds
    mov si, cosic_instruction_base
    mov di, cosic_instruction_argument
    mov ah, 2
    mov al, COSIC_ARGUMENT_SEPERATOR
    mov cx, COSIC_INSTRUCTION_MAX_LENGTH
    int 0xd4 ; Split string

    ; Check for command (frequently used commands at the top to save time)
    push dx
        mov dl, [cs:cosic_inst_variable_int]
        cmp byte [ds:cosic_instruction_base], dl
        jz .init_numeric
    pop dx

    mov ah, 2 ; check if strings are equal caseinsensitive
    mov di, cosic_inst_add
    int 0xd2 ; Check if strings are equal
    ;jnc .add

    mov ah, 2 ; check if strings are equal caseinsensitive
    mov di, cosic_inst_sub
    int 0xd2 ; Check if strings are equal
    ;jnc .sub

    jmp .error_instruction_not_implemented

    .init_numeric:
        pop dx
        call cosic_init_numeric
        or [cs:.error_code], ax 
        jmp .done

    .add:
        call cosic_add
        jmp .done

    .sub: 
        call cosic_sub
        jmp .done

    .error_instruction_not_implemented:
        or byte [cs:.error_code], COSIC_ERROR_INSTRUCTION_NOT_IMPL

        mov al, 2
        mov si, .inst_not_impl
        int 0x97
        jmp .done
        .inst_not_impl db "INSTRUCTION NOT IMPLEMENTED!!!", 0x0a, 0x0d, 0

    .error_instruction_size:
        or byte [cs:.error_code], COSIC_ERROR_INSTRUCTION_SIZE
        jmp .done

    .error_memory_limit:
        or byte [cs:.error_code], COSIC_ERROR_MEMORY_LIMIT
        jmp .done

    .done:
        pop es
        pop ds
        pop di
        pop si
        pop bp
        pop dx
        pop cx
        pop bx

        mov ax, [cs:.error_code]

        iret

    .error_code dw 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Creates (and initializes) a numeric variable with the given length
; 
; Declaring it
; %<variable_name> size <number of digits>
;
; Initializing it
; %<variable_name> assign <number literal>/%<numeric_variable>
;
; input: 
;   ds:cosic_instruction_argument -> parameters
;
; output:
;   al -> Error code
;   ah -> Column number
;-------------------------------------------------------------------------------
cosic_init_numeric:
    xor ax, ax
    mov word [cs:.value_segment], 0xffff
    mov word [cs:.value_offset], 0xffff

    push es
    push di

    ; base now contains name
    ; argument contains size/assign and value 

    ; separate size/assign and value (splitting value in variable)
    mov si, cosic_instruction_argument
    push ax
        ; check size
        int 0xd0
        mov [cs:.value_size], ax

        ; create string with this size and split one part into it 
        int 0xb8
        mov [cs:.value_segment], bx
        mov [cs:.value_offset], dx

        ; Split ds:cosic_instruction_argument (si) into variable
        push bx
        pop es
        mov di, dx
        mov ah, 2 ; case insensitive
        mov al, ASCII_SP
        mov cx, [cs:.value_size]
        int 0xd4 ; Split string
    pop ax
    ; base now contains size/assign
    ; argument contains name
    ; variable contains value

    ; check if size or assign
    mov byte [cs:.op_mode] , 0

    ; "size" or "assign" is in base
    ; numerical value or variable is in variable
    push ax
    push es
        mov si, cosic_instruction_argument
        push cs
        pop es
        mov di, cosic_inst_assign
        mov ah, 2 ; string equals ignore case
        int 0xd2
    pop es
    pop ax
    jc .after_assign

    mov byte [cs:.op_mode], 1
    jmp .after_op_evaluation

    .after_assign:

    push ax
    push es
        mov si, cosic_instruction_argument
        push cs
        pop es
        mov di, cosic_inst_size
        mov ah, 2 ; string equals ignore case
        int 0xd2
    pop es
    pop ax
    jc .error_assign_or_size

    mov byte [cs:.op_mode], 2
    jmp .after_op_evaluation

    .after_op_evaluation:

    ; for .assign_get_digits and .size_get_digits
    push word [cs:.value_segment]
    pop ds
    mov si, [cs:.value_offset]

    ; Get number of digits to assign to var into cs:.digit_count
    cmp byte [cs:.op_mode], 1
    jnz .its_size
    call .assign_get_digits
    jmp .got_digits
    .its_size:
    call .size_get_digits
    .got_digits: ; into .digit_count

    ; Calculate required size for list entry
    push ax
        ; Calculate required size
        xor ax, ax
        mov al, [cs:.digit_count]
        test al, 1
        jz .even
        ; use ax, if input is 255 increasing al will lead to 0, not to 256!
        inc ax ; if req. size is 5, add 1 to get 3 bytes after division
        .even:
        shr ax, 1 ; divide by 2 -> each byte represents 2 digits

        ; Calculate number of free bytes
        mov dx, COSIC_LIST_INTVARS_ENTRY_LENGTH
        sub dx, COSIC_LIST_INTVARS_VALUE_OFFSET

        cmp ax, dx
        jna .value_in_limits

        .value_not_in_limits:
            sub ax, dx ; ax contains remaining bytes needed
            jmp .size_ok

        .value_in_limits:
            mov ax, 0

        .size_ok:
            mov dx, ax ; remaining bytes needed to dx
    pop ax
                
    ; Create new list element with the size to support the number of digits
    mov ch, COSIC_LIST_TYPE_INTVAR
    ; dx contains additional size to base size due to large numbers
    call cosic_new_list_entry
    ; BX:DX now contains newly created list element
    cmp al, 0
    jnz .done ; on error
                
    ; Write name to list element
    push ds
        push cs
        pop ds
        mov si, cosic_instruction_base + 1 ; base contains name (skip %)
        call cosic_write_name_to_list_element
    pop ds

    cmp byte [cs:.op_mode], 1
    jz .assign
    jmp .size_to_list_element

    .size_get_digits:
        ; DS:SI contains numeric to test
        call cosic_is_numeric_literal
        jnc .size_return_error_numeric

        cmp byte [ds:si], ASCII_MINUS
        jz .size_return_error_negative

        push ax
            int 0xe3 ; transform size string to number

            ; check if size exceeds 255 and throw error, if so
            cmp ax, 255
            ja .size_return_error_size

            ; Store size
            mov [cs:.digit_count], al
        pop ax
        jmp .size_return_digits

        .size_return_error_numeric:
            or al, COSIC_ERROR_EXPECTED_NUMERIC_LITERAL
            mov ah, 0

            pop dx ; pop return address
            
            jmp .done

        .size_return_error_size:
            pop ax
            pop dx ; pop return address
            jmp .error_integer_size

        .size_return_error_negative:
            pop dx ; pop return address
            jmp .error_size_negative

        .size_return_digits:
            ret

    .assign_get_digits:
        ; DS:SI contains numeric to test
        call cosic_is_numeric
        cmp ch, 2
        jnz .assign_check_literal
            push dx
                mov dl, [cs:cosic_inst_variable_int]
                cmp [ds:si], dl
            pop dx
            jnz .assign_error_percentage

            inc si ; skip % sign
            mov ch, COSIC_LIST_TYPE_INTVAR
            call cosic_get_list_element_by_name
            ; BX:DX contains donator list element
            dec si ; restore si

            cmp bx, 0xffff
            jz .assign_error_undefined_symbol

            ; Assign a variable -> Copy digit information from variable
            mov byte [cs:.is_numeric_literal], 0
            push ax
                push es
                push di
                    push bx
                    pop es
                    mov di, dx

                    mov al, [es:di + COSIC_LIST_INTVARS_DIGITS_OFFSET]
                pop di
                pop es

                mov [cs:.digit_count], al
            pop ax

        jmp .assign_return_digits

        .assign_check_literal:
        cmp ch, 1
        jnz .assign_return_error
            mov byte [cs:.is_numeric_literal], 1

            ; Assign a literal -> Count digits
            cmp byte [ds:si], ASCII_MINUS
            jnz .assign_get_digits_literal_positive

            inc si ; Skip minus sign

            .assign_get_digits_literal_positive:
            push ax
                int 0xd0 ; Check length of string
                mov [cs:.digit_count], al ; store number of digits
            pop ax

        jmp .assign_return_digits

        .assign_return_error:
            or al, COSIC_ERROR_EXPECTED_NUMERIC
            mov ah, 0

            pop dx ; pop return address

            jmp .done

        .assign_error_percentage:
            pop dx ; return address
            jmp .error_percentage

        .assign_error_undefined_symbol:
            pop dx ; return address
            jmp .error_undefined_symbol

        .assign_return_digits:
            ret

    .size_to_list_element:
        ; BX:DX contains list element
        ; write size byte to list element
        push ax
        push es
        push di
            push bx
            pop es
            mov di, dx

            mov al, [cs:.digit_count]
            mov [es:di + COSIC_LIST_INTVARS_DIGITS_OFFSET], al
        pop di 
        pop es
        pop ax

        jmp .done

    .assign:
        push bx
        pop es
        mov di, dx

        push word [cs:.value_segment]
        pop ds
        mov si, [cs:.value_offset]

        cmp byte [cs:.is_numeric_literal], 0
        jz .assign_variable
        jmp .assign_literal

        or al, COSIC_ERROR_EXPECTED_NUMERIC
        mov ah, 0

        jmp .done

    .assign_variable:
        ; ES:DI contains list element
        ; DS:SI contains name of list element to copy from
        inc si ; skip % sign
        mov ch, COSIC_LIST_TYPE_INTVAR
        call cosic_get_list_element_by_name

        ; BX:DX contains list element to copy from

        push ax
        push cx
            mov ds, bx
            mov si, dx

            mov al, [ds:si + COSIC_LIST_INTVARS_FLAG_OFFSET]
            mov [es:di  + COSIC_LIST_INTVARS_FLAG_OFFSET], al

            mov al, [ds:si + COSIC_LIST_INTVARS_DIGITS_OFFSET]
            mov [es:di + COSIC_LIST_INTVARS_DIGITS_OFFSET], al

            xor cx, cx
            mov cl, al ; number of digits
            shr cl, 1 ; divide cl by two -> one byte stores two digits
            test al, 1
            jz .assign_even
            inc cl
            .assign_even:
            add si, COSIC_LIST_INTVARS_VALUE_OFFSET
            add di, COSIC_LIST_INTVARS_VALUE_OFFSET
            cld
            rep movsb
        pop cx
        pop ax

        jmp .done

    .assign_literal:
        ; ES:DI contains list element
        ; DS:SI contains numeric value
        push ax
        push cx
        push si
            cmp byte [ds:si], ASCII_MINUS
            jnz .assign_literal_positive

            or byte [es:di + COSIC_LIST_INTVARS_FLAG_OFFSET], COSIC_LIST_INTVARS_FLAG_SIGN
            inc si ; Skip minus sign

            .assign_literal_positive:
            mov al, [cs:.digit_count]
            xor cx, cx
            mov cl, al

            mov [es:di + COSIC_LIST_INTVARS_DIGITS_OFFSET], al ; store number of digits
            add di, COSIC_LIST_INTVARS_VALUE_OFFSET

            ; ds:si contains a numeric string -> all values are 0x30 - 0x39
            ; shifting each byte 4 bits left, gives the high nibble
            ; and each byte with 0x0f to get the low nibble
            ; or high and low nibble to write it to the entry
            .assign_literal_loop:
                mov al, [ds:si]
                shl al, 4 ; get high nibble

                or [es:di], al

                ; test if the written entry was the last one to write
                cmp cl, 1
                jz .assign_after_literal_loop

                inc si
                dec cl ; one digit transferred

                mov al, [ds:si]
                and al, 0x0f ; get low nibble

                or [es:di], al

                inc di ; select next byte to write to
                inc si ; select next nibble to write
            loop .assign_literal_loop
            .assign_after_literal_loop:
        pop si
        pop cx
        pop ax

        jmp .done

    .error_percentage:
        or al, COSIC_ERROR_EXPECTED_PERCENT
        mov ah, 0
        jmp .done

    .error_assign_or_size:
        or al, COSIC_ERROR_EXPECTED_ASSIGN_OR_SIZE
        mov ah, 0
        jmp .done

    .error_size_negative:
        or al, COSIC_ERROR_SIZE_NEGATIVE
        mov ah, 0
        jmp .done

    .error_integer_size:
        or al, COSIC_ERROR_SIZE_INTEGER
        mov ah, 0
        jmp .done

    .error_undefined_symbol:
        or al, COSIC_ERROR_UNDEFINED_SYMBOL
        mov ah, 0
        jmp .done

    .done:
        pop di
        pop es

        cmp word [cs:.value_segment], 0xffff
        jz .no_dealloc_value
        cmp word [cs:.value_offset], 0xffff
        jz .no_dealloc_value
            ; deallocate used string
            push ax
            push bx
            push dx
                mov bx, [cs:.value_segment]
                mov dx, [cs:.value_offset]
                mov ax, [cs:.value_size]
                int 0xb9
            pop dx
            pop bx
            pop ax

        .no_dealloc_value:
        ret

    .value_size dw 0
    .value_segment dw 0
    .value_offset dw 0
    .op_mode db 0 ; 0: no op, 2: size, 1: assign
    .is_numeric_literal db 0 ; 0: is numeric variable, 1: is numeric literal 
    .digit_count db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Adds two numbers together optionally giving the result to a third one or 
; printing it to the display
;
; input: 
;   es:bx -> parameters
;
; output:
;   al -> Error code
;   ah -> Column number
;-------------------------------------------------------------------------------
cosic_add:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Subtracts two numbers optionally giving the result to a third one or 
; printing it to the display
; sub <number> from <number> (giving %<var>/display)
;
; input: 
;   es:bx -> parameters
;
; output:
;   al -> Error code
;   ah -> Column number
;-------------------------------------------------------------------------------
cosic_sub:
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Splits ds:si into the initialised cosic_instruction_base ignoring case
;
; input:
;   ds:si -> String to split
;   bl -> Split character
;
; output:
;   ds:si -> Split at split character
;   cosic_instruction_base -> Rest of the string (0, if character non existant)
;-------------------------------------------------------------------------------
cosic_split_into_base:
    pusha

    ; Initialize target
    mov cx, COSIC_INSTRUCTION_MAX_LENGTH
    mov di, cosic_instruction_base
    mov al, 0x00
    int 0xd1 ; initialize string

    ; Split ds:si into cosic_instruction_base
    mov di, cosic_instruction_base
    mov ah, 2 ; case insensitive
    mov al, bl
    mov cx, COSIC_INSTRUCTION_MAX_LENGTH
    int 0xd4 ; Split string

    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Splits ds:si into the initialised cosic_instruction_argument ignoring case
;
; input:
;   ds:si -> String to split
;   bl -> Split character
;
; output:
;   ds:si -> Split at split character
;   cosic_instruction_argument -> Rest of the string (0, if character non existant)
;-------------------------------------------------------------------------------
cosic_split_into_argument:
    pusha

    ; Initialize target
    mov cx, COSIC_INSTRUCTION_MAX_LENGTH
    mov di, cosic_instruction_argument
    mov al, 0x00
    int 0xd1 ; initialize string

    ; Split ds:si into cosic_instruction_argument
    mov di, cosic_instruction_argument
    mov ah, 2 ; case insensitive
    mov al, bl
    mov cx, COSIC_INSTRUCTION_MAX_LENGTH
    int 0xd4 ; Split string

    popa
    ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Initializes the COSIC environment
;
; input: 
;   none
;
; output:
;   none
;-------------------------------------------------------------------------------
cosic_init:
    mov al, 0

    push cs
    pop ds
    mov si, .info_init

    int 0x97

    ; Initialize all lists
    mov ch, COSIC_LIST_TYPE_LABEL
    or ch, 0x80
    xor dx, dx
    call cosic_new_list_entry
    cmp al, 0
    jnz .critical_init_error
    mov [cs:cosic_list_labels_segment], bx
    mov [cs:cosic_list_labels_offset], dx
	
    mov ch, COSIC_LIST_TYPE_POINTER
    or ch, 0x80
    xor dx, dx
    call cosic_new_list_entry
    cmp al, 0
    jnz .critical_init_error
    mov [cs:cosic_list_pointer_segment], bx
    mov [cs:cosic_list_pointer_offset], dx
	
    mov ch, COSIC_LIST_TYPE_HANDLE
    or ch, 0x80
    xor dx, dx
    call cosic_new_list_entry
    cmp al, 0
    jnz .critical_init_error
    mov [cs:cosic_list_handles_segment], bx
    mov [cs:cosic_list_handles_offset], dx
	
    mov ch, COSIC_LIST_TYPE_STRVAR
    or ch, 0x80
    xor dx, dx
    call cosic_new_list_entry
    cmp al, 0
    jnz .critical_init_error
    mov [cs:cosic_list_strvars_segment], bx
    mov [cs:cosic_list_strvars_offset], dx
	
    mov ch, COSIC_LIST_TYPE_INTVAR
    or ch, 0x80
    xor dx, dx
    call cosic_new_list_entry
    cmp al, 0
    jnz .critical_init_error
    mov [cs:cosic_list_intvars_segment], bx
    mov [cs:cosic_list_intvars_offset], dx

    ret

    .critical_init_error:
        mov al, 4
        push cs
        pop ds
        mov si, .ciritcal_error
        int 0x97

        .hang_loop:
            cli
            hlt
        jmp .hang_loop

    .info_init db "Initializing COSIC", ASCII_CR, ASCII_LF, 0
    .ciritcal_error db "Critical error while initializing COSIC! System halt!", 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Creates a new list entry
;
; input: 
;   CH -> List type
;        1: label
;        2: pointer
;        4: handle
;        8: string
;       16: int
;       MSB set: init list
;   DX -> List element additional size to base size (0 for default size)
;
; output:
;   AL -> Error code
;   BX -> List segment
;   DX -> List offset
;-------------------------------------------------------------------------------
cosic_new_list_entry:
    push bp
    push es
    push ax

    test ch, COSIC_LIST_TYPE_LABEL
    jz .no_label
        mov ax, COSIC_LIST_LABELS_ENTRY_LENGTH
        add ax, dx
        int 0xb8

        push word [cs:cosic_list_labels_segment]
        pop es
        mov bp, [cs:cosic_list_labels_offset]
        jmp .done_allocate
    .no_label:

    test ch, COSIC_LIST_TYPE_POINTER
    jz .no_pointer
        mov ax, COSIC_LIST_POINTER_ENTRY_LENGTH
        add ax, dx
        int 0xb8

        push word [cs:cosic_list_pointer_segment]
        pop es
        mov bp, [cs:cosic_list_pointer_offset]
        jmp .done_allocate
    .no_pointer:

    test ch, COSIC_LIST_TYPE_HANDLE
    jz .no_handle
        mov ax, COSIC_LIST_HANDLES_ENTRY_LENGTH
        add ax, dx
        int 0xb8
        
        push word [cs:cosic_list_handles_segment]
        pop es
        mov bp, [cs:cosic_list_handles_offset]
        jmp .done_allocate
    .no_handle:

    test ch, COSIC_LIST_TYPE_STRVAR
    jz .no_str
        mov ax, COSIC_LIST_STRVARS_ENTRY_LENGTH
        add ax, dx
        int 0xb8
        
        push word [cs:cosic_list_strvars_segment]
        pop es
        mov bp, [cs:cosic_list_strvars_offset]
        jmp .done_allocate
    .no_str:

    test ch, COSIC_LIST_TYPE_INTVAR
    jz .no_int
        mov ax, COSIC_LIST_INTVARS_ENTRY_LENGTH
        add ax, dx
        int 0xb8
        
        push word [cs:cosic_list_intvars_segment]
        pop es
        mov bp, [cs:cosic_list_intvars_offset]
        jmp .done_allocate
    .no_int:

    pop ax
    or al, COSIC_ERROR_LIST_TYPE_NOT_SUPPORTED
    jmp .done

    .done_allocate:
        pop ax
        cmp bx, 0xffff
        jnz .enough_space
        or al, COSIC_ERROR_MEMORY_LIMIT
        jmp .done

    .enough_space:
        ; set last list element marking
        push es
        push di
            push bx
            pop es
            mov di, dx

            or byte [es:di + COSIC_LIST_FLAG_OFFSET], 1
        pop di
        pop es

        test ch, 0x80
        jnz .done_init

        ; chain this element to the previous last element of the list
        ; list offset in effect is loaded to es:bp
        push dx
        xor dx, dx
        .loop_list_entries:
            test byte [es:bp + COSIC_LIST_FLAG_OFFSET], 1
            jnz .end_of_list

            ; get next list element
            push word [es:bp + COSIC_LIST_NEXT_OFFSET]
            mov dl, byte [es:bp + COSIC_LIST_NEXT_OFFSET + 2]
            mov bp, dx
            pop es
            jmp .loop_list_entries
        .end_of_list:
        pop dx

        ; previous last chain element is loaded to es:bp
        ; assign pointer of next list element and remove flag
        and byte [es:bp + COSIC_LIST_FLAG_OFFSET], 0xfe

        mov [es:bp + COSIC_LIST_NEXT_OFFSET], bx
        mov [es:bp + COSIC_LIST_NEXT_OFFSET + 2], dl

        jmp .done

    .done_init:

    .done:
        pop es
        pop bp
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Write the object name into the list
;
; input: 
;   CH -> List type
;        1: label
;        2: pointer
;        4: handle
;        8: string
;       16: int
;   BX:DX -> List element to write the name to
;   DS:SI -> Name to write to the list element
;
; output:
;   AL -> Error code
;-------------------------------------------------------------------------------
cosic_write_name_to_list_element:
    push ax
        int 0xd0 ; Check size of string into ax
        mov [cs:.string_size], al
    pop ax

    cmp byte [cs:cosic_sourcecode_mode], TRUE
    jz .ds_si_ok
        push ax
        push bx
        push dx

        ; If not working in source code mode, a copy of the variable name needs 
        ; to be created
        xor ax, ax
        mov al, [cs:.string_size]
        inc ax ; add one byte for terminator
        int 0xb8 ; Allocate memory
        dec ax 
        cmp bx, 0xffff
        jz .error_memory

        ; Copy string from ds:si to bx:dx
        push es
        push di
            mov es, bx
            mov di, dx
            mov bx, 0 ; relative start
            mov dx, ax ; relative end
            int 0xd3

            ; use copy of the string in the following code
            push es
            pop ds
            mov si, di
        pop di
        pop es

        pop dx
        pop bx
        pop ax

    ; DS:SI point to the target string
    .ds_si_ok:
        ; Write string size and string address to list entry
        push ax
        push cx
        push es
        push di
            push bx
            pop es
            mov di, dx

            mov al, [cs:.string_size]
            mov [es:di + COSIC_LIST_NAME_LENGTH_OFFSET], al

            mov ax, ds
            mov cx, si
            shr cx, 4 ; Only the upper three nibbles for the offset

            add ax, cx ; segment
            mov [es:di + COSIC_LIST_NAME_OFFSET], ax

            mov ax, si
            and ax, 0x000f ; offset
    
            mov [es:di + COSIC_LIST_NAME_OFFSET + 2], ax
        pop di
        pop es
        pop cx
        pop ax

        jmp .done

    .error_memory:
        pop dx
        pop bx
        pop ax

        or al, COSIC_ERROR_MEMORY_LIMIT
        jmp .done

    .done:
        ret

    .string_size db 0
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Returns the list element of the list type addressed by the name
;
; input: 
;   CH -> List type
;        1: label
;        2: pointer
;        4: handle
;        8: string
;       16: int
;   DS:SI -> Name of list element (without type prefix)
;
; output:
;   BX:DX -> List element offset (BX = DX = 0xffff, if non existant)
;-------------------------------------------------------------------------------
cosic_get_list_element_by_name:
    push es
    push bp
    push ax
    push cx
    push di

    int 0xd0 ; get length of string in ds:si

    xor dx, dx
    mov bx, si ; save si
    cld 

    cmp ch, COSIC_LIST_TYPE_LABEL
    jnz .no_label
        push word [cs:cosic_list_labels_segment]
        pop es
        mov bp, [cs:cosic_list_labels_offset]
        jmp .search_by_name
    .no_label:

    cmp ch, COSIC_LIST_TYPE_POINTER
    jnz .no_pointer
        push word [cs:cosic_list_pointer_segment]
        pop es
        mov bp, [cs:cosic_list_pointer_offset]
        jmp .search_by_name
    .no_pointer:

    cmp ch, COSIC_LIST_TYPE_HANDLE
    jnz .no_handle
        push word [cs:cosic_list_handles_segment]
        pop es
        mov bp, [cs:cosic_list_handles_offset]
        jmp .search_by_name
    .no_handle:

    cmp ch, COSIC_LIST_TYPE_STRVAR
    jnz .no_str
        push word [cs:cosic_list_strvars_segment]
        pop es
        mov bp, [cs:cosic_list_strvars_offset]
        jmp .search_by_name
    .no_str:

    cmp ch, COSIC_LIST_TYPE_INTVAR
    jnz .no_int
        push word [cs:cosic_list_intvars_segment]
        pop es
        mov bp, [cs:cosic_list_intvars_offset]
        jmp .search_by_name
    .no_int:

    or al, COSIC_ERROR_LIST_TYPE_NOT_SUPPORTED
    jmp .done

    .search_by_name:
        ; check if name and string have the same length
        cmp [es:bp + COSIC_LIST_NAME_LENGTH_OFFSET], al
        jnz .next ; impossible to be the same name here

        ; test names (strings may not be zero terminated -> use length to check)
        push es
            ; set es:di to string offset in entry
            mov dx, [es:bp + COSIC_LIST_NAME_OFFSET + 2]
            and dx, 0x000f
            mov di, dx
            push word [es:bp + COSIC_LIST_NAME_OFFSET]
            pop es

            mov cx, ax ; length of string in ds:si
            mov si, bx
            repe cmpsb
        pop es
        jmp .found

        .next:
        ; test if this was the last entry
        test byte [es:bp + COSIC_LIST_FLAG_OFFSET], 1
        jnz .end_of_list

        ; get next list element
        push word [es:bp + COSIC_LIST_NEXT_OFFSET]
        mov dl, byte [es:bp + COSIC_LIST_NEXT_OFFSET + 2]
        mov bp, dx
        pop es
        jmp .search_by_name


    .found:
        ; reset si -> not saved via stack
        mov si, bx

        ; set bx:dx
        mov bx, es
        mov dx, bp
        jmp .done

    .end_of_list:
        ; reset si -> not saved via stack
        mov si, bx

        mov bx, 0xffff
        mov dx, bx
        jmp .done

    .done:
        pop di
        pop cx
        pop ax
        pop bp
        pop es
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Checks if the String at ds:si is a numeric (literal or variable name)
;
; input: 
;   DS:SI -> Literal or variable name (% included in name for variables!)
;
; output:
;   ch -> Flags
;       0: not numeric
;       1: numeric (literal)
;       2: numeric (variable)
;   BX:DX offset of list element, if variable
;-------------------------------------------------------------------------------
cosic_is_numeric:
    mov ch, 1 ; numeric literal
    call cosic_is_numeric_literal
    jc .done

    mov ch, 2 ; numeric variable
    push si
    push cx
        mov ch, COSIC_LIST_TYPE_INTVAR
        inc si ; ignore % for check 
        call cosic_get_list_element_by_name
    pop cx
    pop si
    cmp bx, 0xffff
    jz .done

    mov ch, 2

    .done:
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Checks if the String at ds:si is a numeric literal
;
; input: 
;   DS:SI -> Literal or variable name
;
; output:
;   Carry set, if numeric literal
;-------------------------------------------------------------------------------
cosic_is_numeric_literal:
    push si

    cmp byte [ds:si], 0
    jz .not_numeric

    cmp byte [ds:si], ASCII_MINUS
    jnz .loop

    inc si

    .loop:
        cmp byte [ds:si], 0
        jz .numeric

        cmp byte [ds:si], ASCII_0
        jb .not_numeric

        cmp byte [ds:si], ASCII_9
        ja .not_numeric

        inc si
    jmp .loop

    .not_numeric:
        clc
        jmp .done

    .numeric:
        stc

    .done:
        pop si
        ret
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Execute COSIC Program
;
; input: 
;   DS:SI -> Path to COSIC Program
;
; output:
;   AL -> Error Code
;   AH -> Column Number
;   DX -> Line number
;-------------------------------------------------------------------------------
cosic_run_program:
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
;    ret
;-------------------------------------------------------------------------------
