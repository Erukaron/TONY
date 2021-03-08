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
; Fallout hacking game
;-------------------------------------------------------------------------------
;%define _USE_TEXT_MODE

; 20 passwords per round
; 34 game lines
; 12 chars per block
; 2 blocks
; => 12 * 34 * 2 = 816 characters
; 0xF650 0123456789AB  0xF71C CDEF01234567
; .
; .
; .
; 0xF710 0123456789AB  0xF7DC CDEF01234567  >>0123456789ABCDEF

FO_PW_LEN_LV_0  equ 5
FO_PW_LEN_LV_1  equ 8
FO_PW_LEN_LV_2  equ 10
FO_PW_LEN_LV_3  equ 12
FO_PW_LEN_LV_4  equ 15
FO_PW_LEN_MAX   equ FO_PW_LEN_LV_4
fo_pw_len_lv_current db 0

FO_NO_OF_PWS                equ 20
FO_PREFIX_CHARS             equ 1
FO_SUFFIX_CHARS             equ 1
FO_CHARS_PER_LINE           equ 22
%ifndef _USE_TEXT_MODE
    FO_LINES_PER_BLOCK      equ 28
%else
    FO_LINES_PER_BLOCK      equ 20
%endif
FO_NO_OF_BLOCKS             equ 2
FO_MAX_CHAR_COUNT           equ FO_NO_OF_BLOCKS * FO_LINES_PER_BLOCK * FO_CHARS_PER_LINE
FO_BLOCK_CHARS              equ FO_CHARS_PER_LINE + 6 + 1 + 2 ; 6 = hex number ; 1 = space between hex and pw ; 2 = space to next block

FO_NO_OF_FILL_CHARS_LV_0    equ FO_MAX_CHAR_COUNT - (FO_SUFFIX_CHARS + FO_PREFIX_CHARS + FO_PW_LEN_LV_0) * FO_NO_OF_PWS
FO_NO_OF_FILL_CHARS_LV_1    equ FO_MAX_CHAR_COUNT - (FO_SUFFIX_CHARS + FO_PREFIX_CHARS + FO_PW_LEN_LV_1) * FO_NO_OF_PWS
FO_NO_OF_FILL_CHARS_LV_2    equ FO_MAX_CHAR_COUNT - (FO_SUFFIX_CHARS + FO_PREFIX_CHARS + FO_PW_LEN_LV_2) * FO_NO_OF_PWS
FO_NO_OF_FILL_CHARS_LV_3    equ FO_MAX_CHAR_COUNT - (FO_SUFFIX_CHARS + FO_PREFIX_CHARS + FO_PW_LEN_LV_3) * FO_NO_OF_PWS
FO_NO_OF_FILL_CHARS_LV_4    equ FO_MAX_CHAR_COUNT - (FO_SUFFIX_CHARS + FO_PREFIX_CHARS + FO_PW_LEN_LV_4) * FO_NO_OF_PWS
fo_no_of_fill_chars_lv_current dw 0

FO_AVERAGE_FILL_CHAR_NO_LV_0    equ FO_NO_OF_FILL_CHARS_LV_0 / FO_NO_OF_PWS
FO_AVERAGE_FILL_CHAR_NO_LV_1    equ FO_NO_OF_FILL_CHARS_LV_1 / FO_NO_OF_PWS
FO_AVERAGE_FILL_CHAR_NO_LV_2    equ FO_NO_OF_FILL_CHARS_LV_2 / FO_NO_OF_PWS
FO_AVERAGE_FILL_CHAR_NO_LV_3    equ FO_NO_OF_FILL_CHARS_LV_3 / FO_NO_OF_PWS
FO_AVERAGE_FILL_CHAR_NO_LV_4    equ FO_NO_OF_FILL_CHARS_LV_4 / FO_NO_OF_PWS

%ifndef _USE_TEXT_MODE
    FO_MAX_FILL_CHARS_LV_0  equ 0b1101111 ; 111
    FO_MAX_FILL_CHARS_LV_1  equ 0b1011111 ; 95
    FO_MAX_FILL_CHARS_LV_2  equ 0b1001111 ; 79
    FO_MAX_FILL_CHARS_LV_3  equ 0b1000111 ; 71
    FO_MAX_FILL_CHARS_LV_4  equ 0b0111111 ; 63
%else
    FO_MAX_FILL_CHARS_LV_0  equ 0b111111 ; 63
    FO_MAX_FILL_CHARS_LV_1  equ 0b1111111 ; 127
    FO_MAX_FILL_CHARS_LV_2  equ 0b1111111 ; 127
    FO_MAX_FILL_CHARS_LV_3  equ 0b1111111 ; 127
    FO_MAX_FILL_CHARS_LV_4  equ 0b1111111 ; 127
%endif
fo_max_fill_chars_lv_current db 0

fo_fill_characters_left             dw 0
fo_no_of_fill_chars_current_round   db 0

FO_HEX_OFFSET_START equ 0xf650
fo_hex_offset   dw 0x0000

FO_OBFUSCATOR_CHARS_COUNT   equ 28
fo_obfuscator_char_list db "^!§$%&/()=?*+#°-_.,:;|<>\}][{"

fo_display_data times (FO_MAX_CHAR_COUNT + 1) db 0

fo_pw_length        db FO_PW_LEN_LV_0, FO_PW_LEN_LV_1, FO_PW_LEN_LV_2, FO_PW_LEN_LV_3, FO_PW_LEN_LV_4
fo_random_pw_id     db 0
fo_real_pw_written  db FALSE

fo_entered_passwords_list           times 4 * (FO_PW_LEN_MAX + 1) db 0 ; all entered passwords
fo_entered_passwords_matches_list   times 4 db 0

FO_PW_TRIES     equ 4

fo_pw_list_used_pw_ids times (FO_NO_OF_PWS - 1) db 0xff ; exclude the correct password -> has its own variable

; 40 entries per list
FO_PW_LIST_ENTRIES equ 40

fo_pw_list_offset_current dw 0 

fo_pw_list_0    db "FUZZY"
                db "MUZZY"
                db "WHIZZ"
                db "DIZZY"
                db "PIZZA"
                db "JACKY"
                db "JIMMY"
                db "FUNKY"
                db "ZAPPY"
                db "GIZMO"
                db "JOKED"
                db "KANJI"
                db "QUAKE"
                db "XEROX"
                db "BANJO"
                db "BEZEL"
                db "BLITZ"
                db "CHUCK"
                db "EQUIP"
                db "FAZED"
                db "FURZE"
                db "JAPAN"
                db "JEWEL"
                db "JUDGE"
                db "KAZOO"
                db "WALTZ"
                db "FIXED"
                db "CHICK"
                db "KNOCK"
                db "PROXY"
                db "QUILL"
                db "VEXIL"
                db "BOXED"
                db "CHAMP"
                db "CHEVY"
                db "CODEX"
                db "COMFY"
                db "DOZEN"
                db "EQUAL"
                db "EXPEL"
                db "HAMZA"

fo_pw_list_1    db "THIRTEEN"
                db "THURSDAY"
                db "PRINCESS"
                db "FALLOUT4"
                db "THOUSAND"
                db "FOURTEEN"
                db "LANGUAGE"
                db "CHIPOTLE"
                db "AMERICAN"
                db "BUSINESS"
                db "FAVORITE"
                db "ELEPHANT"
                db "CHILDREN"
                db "BIRTHDAY"
                db "MOUNTAIN"
                db "FEMININE"
                db "FOOTBALL"
                db "KINDNESS"
                db "SYLLABLE"
                db "ABDICATE"
                db "TREASURE"
                db "VIRGINIA"
                db "ENVELOPE"
                db "STRENGTH"
                db "TOGETHER"
                db "MEMORIES"
                db "DARKNESS"
                db "FEBRUARY"
                db "SANDWICH"
                db "CALENDAR"
                db "BULLYING"
                db "EQUATION"
                db "VIOLENCE"
                db "MARRIAGE"
                db "BUILDING"
                db "INTERNAL"
                db "FUNCTION"
                db "NOVEMBER"
                db "VICTORIA"
                db "ABORTION"

fo_pw_list_2    db "CALIFORNIA"
                db "EVERYTHING"
                db "WASHINGTON"
                db "BASKETBALL"
                db "WEATHERING"
                db "CHARACTERS"
                db "LITERATURE"
                db "PERFECTION"
                db "VOLLEYBALL"
                db "DEPRESSION"
                db "HOMECOMING"
                db "TECHNOLOGY"
                db "MALEFICENT"
                db "WATERMELON"
                db "APPRECIATE"
                db "RELAXATION"
                db "CONVECTION"
                db "GOVERNMENT"
                db "ABOMINABLE"
                db "SALMONELLA"
                db "STRAWBERRY"
                db "ABERRATION"
                db "RETIREMENT"
                db "TELEVISION"
                db "ALZHEIMERS"
                db "SILHOUETTE"
                db "FRIENDSHIP"
                db "LONELINESS"
                db "PUNISHMENT"
                db "UNIVERSITY"
                db "CONFIDENCE"
                db "ABSTINENCE"
                db "DISCIPLINE"
                db "HELICOPTER"
                db "GENERATION"
                db "SKATEBOARD"
                db "APOCALYPSE"
                db "LEADERSHIP"
                db "REVOLUTION"
                db "INSULATION"

fo_pw_list_3    db "ALLITERATION"
                db "UNIDENTIFIED"
                db "INTERMITTENT"
                db "PENNSYLVANIA"
                db "EXACERBATION"
                db "INDEPENDENCE"
                db "COMMENSALISM"
                db "INTELLIGENCE"
                db "RELATIONSHIP"
                db "THANKSGIVING"
                db "PROFESSIONAL"
                db "ORGANIZATION"
                db "SPORADICALLY"
                db "INTIMIDATING"
                db "APPRECIATION"
                db "ANNUNCIATION"
                db "BIODIVERSITY"
                db "ACCELERATION"
                db "INTERDICTION"
                db "TRIGONOMETRY"
                db "COMMUNICATOR"
                db "BODYBUILDING"
                db "PERSPIRATION"
                db "RESURRECTION"
                db "CIVILIZATION"
                db "VELOCIRAPTOR"
                db "EXPECTATIONS"
                db "AMBIDEXTROUS"
                db "MAGAZINCATCH"
                db "HIPPOPOTAMUS"
                db "INTERJECTION"
                db "FELICITATION"
                db "ABBREVIATION"
                db "VOLUMINOSITY"
                db "COLONIZATION"
                db "INTERCEPTION"
                db "CHAMPIONSHIP"
                db "ACQUAINTANCE"
                db "CONSTIPATION"
                db "CYTOSKELETON"

fo_pw_list_4    db "PROCRASTINATION"
                db "PERSONIFICATION"
                db "CHARACTERISTICS"
                db "DESERTIFICATION"
                db "ABDOMINOVESICAL"
                db "CONGRATULATIONS"
                db "INFLUENTIALNESS"
                db "THERMOCHEMISTRY"
                db "ALPHABETISATION"
                db "METHAMPHETAMINE"
                db "INTERDEPENDENCE"
                db "ACANTHISITTIDAE"
                db "ACCOMPLISHMENTS"
                db "ACKNOWLEDGEABLE"
                db "AMERICANIZATION"
                db "ACKNOWLEDGEMENT"
                db "PROGNOSTICATION"
                db "ACCLIMATIZATION"
                db "MANEUVERABILITY"
                db "RATIONALISATION"
                db "BIOLUMINESCENCE"
                db "SYLLABIFICATION"
                db "MISCHIEVOUSNESS"
                db "PARTHENOGENESIS"
                db "DISCOMBOBULATED"
                db "TRUSTWORTHINESS"
                db "CARNIVOROUSNESS"
                db "ARMADILLIDIIDAE"
                db "NATIONALISATION"
                db "KINDHEARTEDNESS"
                db "EXCOMMUNICATION"
                db "PROCRASTENATING"
                db "MEDICAMENTATION"
                db "CONFIDENTIALITY"
                db "INTERNALIZATION"
                db "MISAPPRECIATION"
                db "ACADEMICIANSHIP"
                db "ANTICHOLINERGIC"
                db "APPROPRIATENESS"
                db "TOTALITARIANISM"
                
fo_fg_color db 50
fo_bg_color db 194

fo_input_char   db ">"

fo1_output_1    db "WELCOME TO ROBCO INDUSTRIES (TM) TERMLINK", ASCII_CR, ASCII_LF, 0
fo1_input_1     db "SET TERMINAL/INQUIRE", 0
fo1_output_2    db "RIT-V300", ASCII_CR, ASCII_LF, 0
fo1_input_2     db "SET FILE/PROTECTION=OWNER:RWED ACCOUNTS.F", 0
fo1_input_3     db "SET HALT RESTART/MAINT", 0
fo1_output_3    db "Initializing Robco Industries(TM) MF Boot Agent v2.3.0", ASCII_CR, ASCII_LF
                db "RETROS BIOS", ASCII_CR, ASCII_LF
                db "RBIOS-4.02.08.00 52EE5.E7.E8", ASCII_CR, ASCII_LF
                db "Copyright 2201-2203 Robco Ind.", ASCII_CR, ASCII_LF
                db "Uppermem: 64 KB", ASCII_CR, ASCII_LF
                db "Root (5A8)", ASCII_CR, ASCII_LF
                db "Maintenance Mode", ASCII_CR, ASCII_LF, 0
fo1_input_4     db "RUN DEBUG/ACCOUNTS.F", 0
fo2_output_1    db "ROBCO INDUSTRIES (TM) TERMLINK PROTOCOL", ASCII_CR, ASCII_LF
                db "ENTER PASSWORD NOW", ASCII_CR, ASCII_LF, ASCII_CR, ASCII_LF, ASCII_CR, ASCII_LF, 0
fo2_output_pw   db " ATTEMPT(S) LEFT: ", 0
fo2_pw_fail_1   db "Entry denied", 0
fo2_pw_fail_2   db " correct.   ", 0 ; spaces to overwrite existing data
fo3_lockdown_1  db "TERMINAL LOCKED", 0
fo3_lockdown_2  db "PLEASE CONTACT AN ADMINISTRATOR", 0
fo3_success     db "Access Granted.", 0

FO_PW_INDICATOR_X               equ 0
FO_PW_INDICATOR_Y               equ 3
FO_PW_INDICATOR_TEXT_OFFSET     equ 19

FO_BLOCK1_X       equ 0
FO_BLOCK1_Y       equ 5

FO_USER_PROMPT_X  equ FO_BLOCK_CHARS * FO_NO_OF_BLOCKS
FO_USER_PROMPT_Y  equ FO_LINES_PER_BLOCK + 4

FO_LOCKDOWN1_X  equ 32
FO_LOCKDOWN2_X  equ 24
%ifndef _USE_TEXT_MODE
    FO_LOCKDOWN1_Y  equ 13
    FO_LOCKDOWN2_Y  equ 15
%else
    FO_LOCKDOWN1_Y  equ 11
    FO_LOCKDOWN2_Y  equ 13
%endif
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Fallout login hacking game
;-------------------------------------------------------------------------------
fallout_hacking_game:
    %ifndef _USE_TEXT_MODE
        mov ah, [cs:fo_bg_color]
        mov al, [cs:fo_fg_color]
        int 0x9a ; set color
    %else
        mov al, VIDEOMODE_ASCII_COLOR_80x25
        int 0x93
    %endif

    int 0x95 ; clear screen

    mov si, fo2_output_1
    call fo_display_line

    mov al, [ds:fo_mode]
    cmp al, 0
    jz fo_mode_match
    cmp al, 1
    jz fo_mode_keystroke
    cmp al, 2
    jz fo_mode_auto

fo_mode_auto:
    mov si, fo1_output_1
    call fo_display_line
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_1
    call fo_display_line
    mov al, ASCII_LF
    call fo_putch ; display character
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov si, fo1_output_2
    call fo_display_line
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_2
    call fo_display_line
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_3
    call fo_display_line
    mov al, ASCII_LF
    call fo_putch ; display character

    mov ax, 2000
    int 0x81 ; sleep ax milli seconds

    mov si, fo1_output_3
    call fo_display_line
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_4
    call fo_display_line
    mov ax, 2000
    int 0x81 ; sleep ax milli seconds

    int 0x95 ; cls

    mov si, fo2_output_1
    call fo_display_line

    call fo_game

    ret
fo_mode_keystroke:
    mov si, fo1_output_1
    call fo_display_line

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_1
    call fo_fake_line_entry
    mov al, ASCII_LF
    call fo_putch ; display character

    mov si, fo1_output_2
    call fo_display_line

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_2
    call fo_fake_line_entry
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_3
    call fo_fake_line_entry
    mov al, ASCII_LF
    call fo_putch ; display character

    mov ax, 2000
    int 0x81 ; sleep ax milli seconds

    mov si, fo1_output_3
    call fo_display_line

    mov al, [cs:fo_input_char]
    call fo_putch ; display character
    mov si, fo1_input_4
    call fo_fake_line_entry
    mov ax, 2000
    int 0x81 ; sleep ax milli seconds

    int 0x95 ; cls

    mov si, fo2_output_1
    call fo_display_line

    call fo_game

    ret
fo_mode_match:
    mov si, fo1_output_1
    call fo_display_line

    mov si, fo1_input_1
    call fo_check_line_entry
    mov al, ASCII_LF
    call fo_putch ; display character

    mov si, fo1_output_2
    call fo_display_line

    mov si, fo1_input_2
    call fo_check_line_entry
    mov ax, 500
    int 0x81 ; sleep ax milli seconds

    mov si, fo1_input_3
    call fo_check_line_entry
    mov al, ASCII_LF
    call fo_putch ; display character

    mov ax, 2000
    int 0x81 ; sleep ax milli seconds

    mov si, fo1_output_3
    call fo_display_line

    mov si, fo1_input_4
    call fo_check_line_entry
    mov ax, 2000
    int 0x81 ; sleep ax milli seconds

    int 0x95 ; cls

    mov si, fo2_output_1
    call fo_display_line

    call fo_game

    ret

; Print ds:si to next terminator
; linebreak at end
fo_display_line_no_ln:
    cmp byte [ds:si], 0
    jz .done

    mov al, [ds:si]
    inc si
    call fo_putch

    jmp fo_display_line_no_ln

    .done:
        ret

; Print ds:si to next terminator
; linebreak at end
fo_display_line:
    call fo_display_line_no_ln

    push ax
        mov al, ASCII_CR
        int 0x90 ; display character
        mov al, ASCII_LF
        int 0x90 ; display character
    pop ax

    ret

; Print ds:si to next terminator at keystroke
; linebreak at end
fo_fake_line_entry:
    int 0xb5 ; clear keyboard buffer

    cmp byte [ds:si], 0
    jz .done

    push ax
        mov ah, 0xff ; do not display char
        int 0xb0 ; wait for keystrok

        mov al, [ds:si]
        int 0x90 ; display character
        inc si
    pop ax

    jmp fo_fake_line_entry

    .done:
        push ax
            mov al, ASCII_CR
            int 0x90 ; display character
            mov al, ASCII_LF
            int 0x90 ; display character
        pop ax

        ret

; Checks if the entered string matches to ds:si
; returns if so, display an error message if not and accepts input again
fo_check_line_entry:
    int 0xb5 ; clear keyboard buffer

    pusha
        mov al, [cs:fo_input_char]
        call fo_putch ; display character

        mov cx, .BUFFER_SIZE
        mov di, .buffer
        int 0xb1

        mov ah, 0b11 ; string equal, ignore case
        int 0xd2
    popa

    jnc .done
        push si
            mov si, .error_message
            call fo_display_line
        pop si
    jmp fo_check_line_entry

    .done:
        ret

    .error_message db "Unknown file or command!", 0
    .BUFFER_SIZE equ 79
    .buffer times .BUFFER_SIZE db 0

fo_game:
    ; set start address
    mov ax, FO_HEX_OFFSET_START
    mov [ds:fo_hex_offset], ax

    ; Clear selected passwords:
    mov di, fo_pw_list_used_pw_ids
    mov cx, (FO_NO_OF_PWS - 1) 
    .clear_pw_list:
        mov byte [es:di], 0xff
        inc di
        loop .clear_pw_list

    ; Generate random "correct" password
    int 0xf4 ; get random byte into al
    and al, 0b111111 ; maximum is 64
    cmp al, FO_PW_LIST_ENTRIES
    jb .skip_adjust_al_create_pw
        sub al, FO_PW_LIST_ENTRIES
    .skip_adjust_al_create_pw:
    ; al is between 0 and 39
    mov [cs:fo_random_pw_id], al
    
    ; generate random "fake" password ids
    mov cx, (FO_NO_OF_PWS - 1)
    mov di, fo_pw_list_used_pw_ids
    .loop_create_fake_pws:
        int 0xf4 ; get random byte into al
        and al, 0b111111 ; maximum is 64
        cmp al, FO_PW_LIST_ENTRIES
        jb .skip_adjust_al_fake_pws
            sub al, FO_PW_LIST_ENTRIES
        .skip_adjust_al_fake_pws:
        ; al is between 0 and 39

        ; Check if password is unequal to correct password
        cmp [cs:fo_random_pw_id], al
        jz .loop_create_fake_pws

        ; Check if list already contains generated pw
        mov si, fo_pw_list_used_pw_ids
        push cx
            mov cx, (FO_NO_OF_PWS - 1)
            .loop_check_pw_exists:
                cmp [ds:si], al
                jz .pw_exists

                inc si
                loop .loop_check_pw_exists
        pop cx

        ; pw does not exist, write it to list
        mov [es:di], al
        inc di
    loop .loop_create_fake_pws

    mov di, fo_display_data

    cmp byte [ds:fo_pw_level], 4
    jnz .not_4
        mov word [ds:fo_no_of_fill_chars_lv_current], FO_NO_OF_FILL_CHARS_LV_4
        mov byte [ds:fo_max_fill_chars_lv_current], FO_MAX_FILL_CHARS_LV_4
        mov word [ds:fo_pw_list_offset_current], fo_pw_list_4
        mov byte [ds:fo_pw_len_lv_current], FO_PW_LEN_LV_4
        jmp .display_var_filled
    .not_4:
    cmp byte [ds:fo_pw_level], 3
    jnz .not_3
        mov word [ds:fo_no_of_fill_chars_lv_current], FO_NO_OF_FILL_CHARS_LV_3
        mov byte [ds:fo_max_fill_chars_lv_current], FO_MAX_FILL_CHARS_LV_3
        mov word [ds:fo_pw_list_offset_current], fo_pw_list_3
        mov byte [ds:fo_pw_len_lv_current], FO_PW_LEN_LV_3
        jmp .display_var_filled
    .not_3:
    cmp byte [ds:fo_pw_level], 2
    jnz .not_2
        mov word [ds:fo_no_of_fill_chars_lv_current], FO_NO_OF_FILL_CHARS_LV_2
        mov byte [ds:fo_max_fill_chars_lv_current], FO_MAX_FILL_CHARS_LV_2
        mov word [ds:fo_pw_list_offset_current], fo_pw_list_2
        mov byte [ds:fo_pw_len_lv_current], FO_PW_LEN_LV_2
        jmp .display_var_filled
    .not_2:
    cmp byte [ds:fo_pw_level], 1
    jnz .not_1
        mov word [ds:fo_no_of_fill_chars_lv_current], FO_NO_OF_FILL_CHARS_LV_1
        mov byte [ds:fo_max_fill_chars_lv_current], FO_MAX_FILL_CHARS_LV_1
        mov word [ds:fo_pw_list_offset_current], fo_pw_list_1
        mov byte [ds:fo_pw_len_lv_current], FO_PW_LEN_LV_1
        jmp .display_var_filled
    .not_1:

        mov word [ds:fo_no_of_fill_chars_lv_current], FO_NO_OF_FILL_CHARS_LV_0
        mov byte [ds:fo_max_fill_chars_lv_current], FO_MAX_FILL_CHARS_LV_0
        mov word [ds:fo_pw_list_offset_current], fo_pw_list_0
        mov byte [ds:fo_pw_len_lv_current], FO_PW_LEN_LV_0
    .display_var_filled:
    call .build_level

    ;Display data in blocks
    mov si, fo_display_data

    ;mov ah, 1
    ;int 0x98 ; get current screen position into bx and dx
    ;mov [ds:.x_org], bx
    ;mov [ds:.y_org], dx
    mov bx, FO_BLOCK1_X
    mov dx, FO_BLOCK1_Y
    mov [ds:.x_org], bx
    mov [ds:.y_org], dx
    mov ah, 1
    int 0x99 ; set screen position to bx/dx

    ; Block 1
    mov cx, FO_LINES_PER_BLOCK
    .block1_dsp:
        push cx
            mov cx, FO_CHARS_PER_LINE

            call .display_hex_prefix
                    
            .block1_line_dsp:
                mov al, [ds:si]
                inc si
                call fo_putch
            loop .block1_line_dsp
        pop cx

        mov bx, [ds:.x_org]
        inc dx
        mov ah, 1
        int 0x99 ; set screen position to bx/dx
    loop .block1_dsp

    mov bx, [ds:.x_org]
    add bx, FO_BLOCK_CHARS
    mov [ds:.x_org], bx
    mov dx, [ds:.y_org]
    mov ah, 1
    int 0x99 ; set screen position to bx/dx

    ; Block 2
    mov cx, FO_LINES_PER_BLOCK
    .block2_dsp:
        push cx
            mov cx, FO_CHARS_PER_LINE

            call .display_hex_prefix
                    
            .block2_line_dsp:
                mov al, [ds:si]
                inc si
                call fo_putch
            loop .block2_line_dsp
        pop cx

        mov bx, [ds:.x_org]
        inc dx
        mov ah, 1
        int 0x99 ; set screen position to bx/dx
    loop .block2_dsp

    ; Display Attempts left
    mov bx, FO_PW_INDICATOR_X
    mov dx, FO_PW_INDICATOR_Y
    add bx, 2 ; ascii number and space
    mov ah, 1
    int 0x99 ; set screen position to bx/dx
    mov si, fo2_output_pw
    call fo_display_line_no_ln
    ; display 4 symbols
    mov cx, 4
    .loop_tries_symbols:
        mov al, ASCII_BLOCK
        int 0x90
        mov al, ASCII_SP
        int 0x90
    loop .loop_tries_symbols
    
    ; User prompt
    mov bx, FO_USER_PROMPT_X
    mov [ds:.x_org], bx
    mov dx, FO_USER_PROMPT_Y
    mov [ds:.y_org], dx
    mov cx, FO_PW_TRIES
    .try_pw:
        ; display attempts:
        mov bx, FO_PW_INDICATOR_X
        mov dx, FO_PW_INDICATOR_Y
        mov ah, 1
        int 0x99 ; set screen position to bx/dx
        mov dl, cl
        int 0xe4 ; transform remaining tries into ascii
        mov al, dl
        call fo_putch

        mov bx, [ds:.x_org]
        mov dx, [ds:.y_org]
        mov ah, 1
        int 0x99 ; set screen position to bx/dx

        mov al, [ds:fo_input_char]
        call fo_putch

        push bx
        push cx
            ; calculate password offset
            mov ax, cx
            mov ah, FO_PW_TRIES
            sub ah, al
            mov al, ah
            xor ah, ah
            mov cl, (FO_PW_LEN_MAX + 1)
            mul cl
            mov di, fo_entered_passwords_list
            add di, ax

            ; calculate number of characters
            xor bx, bx
            xor cx, cx
            mov bl, [ds:fo_pw_level]
            mov cl, [ds:fo_pw_length + bx]
            add cl, 1 ; -> adjust for zero terminator

            int 0xb5
            call fo_read_no_linebreak

            ; transform input to upper
            mov si, di
            push cx
                mov ah, 0
                mov cx, 0xffff
                int 0xd7 ; SI to upper
            pop cx
        pop cx
        pop bx

        call .check_password
        jnc .login ; if pw ok -> login
        ; if pw not ok 
        ; shift entry block 3 lines up
        mov bp, cx ; remaining tries
        push cx
            ; number of shifts is FO_PW_TRIES - (cx - 1)
            sub cx, FO_PW_TRIES + 1
            not cx
            inc cx
            mov dx, FO_USER_PROMPT_Y - 1 ; write newest password first

            ; first index to address to read the entered password
            mov [ds:.pw_list_index], cl

            .shift_block_3_up_loop:
                dec byte [ds:.pw_list_index]    ; first run : adjust index to point to offset 0
                                                ; other runs: decrement the offset -> last entered passwords are displayed first, because the screen is built up from bottom to top

                ; Write ">x/y correct."
                push dx
                    mov bx, FO_USER_PROMPT_X
                    mov ah, 1
                    int 0x99 ; set screen position to bx/dx

                    ; ">"
                    mov al, [ds:fo_input_char]
                    int 0x90 ; putch

                    ; "x"
                    mov al, [ds:.pw_list_index]
                    xor ah, ah
                    mov si, fo_entered_passwords_matches_list
                    add si, ax
                    mov dl, [ds:si]
                    int 0xe4 ; transform dl to printable hex
                    mov al, dl
                    int 0x90 ; putch               

                    ; "/"
                    mov al, ASCII_SLASH
                    int 0x90 ; putch

                    ; "y"
                    xor dx, dx
                    xor bx, bx
                    mov bl, [ds:fo_pw_level]
                    mov dl, [ds:fo_pw_length + bx]
                    int 0xe4 ; transform dl to printable hex
                    mov al, dl
                    int 0x90 ; putch               

                    ; " correct."
                    mov si, fo2_pw_fail_2
                    int 0x91
                pop dx
                dec dx

                ; Write ">Entry denied"
                mov bx, FO_USER_PROMPT_X
                mov ah, 1
                int 0x99 ; set screen position to bx/dx

                ; ">"
                mov al, [ds:fo_input_char]
                int 0x90 ; putch

                ; "Entry denied"
                mov si, fo2_pw_fail_1
                int 0x91
                dec dx

                ; Write ">"old password
                push cx
                push dx
                    mov bx, FO_USER_PROMPT_X
                    mov ah, 1
                    int 0x99 ; set screen position to bx/dx

                    ; ">"
                    mov al, [ds:fo_input_char]
                    int 0x90 ; putch

                    ; get number of characters per password into cx
                    xor ax, ax
                    xor bx, bx
                    mov bl, [ds:fo_pw_level]
                    mov al, [ds:fo_pw_length + bx]
                    mov cx, ax

                    ; get entered password into si
                    push cx
                        mov al, [ds:.pw_list_index]
                        xor ah, ah
                        mov si, fo_entered_passwords_list
                        xor cx, cx
                        mov cl, (FO_PW_LEN_MAX + 1)
                        mul cl
                        add si, ax
                    pop cx

                    push cx ; save for the next step
                        .shift_write_pw:
                            mov al, [ds:si]
                            inc si
                            int 0x90 ; putch
                        loop .shift_write_pw
                    pop cx

                    ; overwrite exactly 16 bytes ">"+15byte pw
                    sub cx, FO_PW_LEN_MAX
                    not cx
                    inc cx
                    cmp cx, 0
                    jz .shift_skip_filling_pw
                    mov al, ASCII_SP
                    .shift_fill_pw:
                        int 0x90 ; putch
                    loop .shift_fill_pw
                    .shift_skip_filling_pw:
                pop dx
                pop cx
                dec dx
            dec cx
            cmp cx, 0
            jnz .shift_block_3_up_loop
        pop cx

        ; clear the last line
        push cx
            mov cx, FO_PW_LEN_MAX
            inc cx ; for ">"
            mov dx, FO_USER_PROMPT_Y
            mov bx, FO_USER_PROMPT_X
            mov ah, 1
            int 0x99 ; set screen position to bx/dx
            mov al, ASCII_SP
            .loop_clear_block_3_last_line:
                int 0x90 ; putch
            loop .loop_clear_block_3_last_line
        pop cx

        ; delete current last symbol of pw count
        mov bx, FO_PW_INDICATOR_X + FO_PW_INDICATOR_TEXT_OFFSET
        push cx
            shl cx, 1 ; multiply by 2 -> space and character
            add bx, cx
        pop cx
        dec bx ; adjust for last space
        mov dx, FO_PW_INDICATOR_Y
        mov ah, 1
        int 0x99 ; set screen position to bx/dx
        mov al, ASCII_SP
        call fo_putch ; overwrite one try
    dec cx
    cmp cx, 0
    jnz .try_pw

    ; terminal lockdown
    int 0x95 ; cls

    mov bx, FO_LOCKDOWN1_X
    mov dx, FO_LOCKDOWN1_Y
    mov ah, 1
    int 0x99 ; set cursors

    mov si, fo3_lockdown_1
    call fo_display_line_no_ln

    mov bx, FO_LOCKDOWN2_X
    mov dx, FO_LOCKDOWN2_Y
    mov ah, 1
    int 0x99 ; set cursors

    mov si, fo3_lockdown_2
    call fo_display_line_no_ln

    cli
    hlt
    jmp $-1

    ; returns carry flag set, if password is not correct
    ; cx needs to be set to the remaining password tries
    .check_password:
        pusha
            ; calculate password offset
            mov ax, cx
            mov ah, FO_PW_TRIES
            sub ah, al
            mov al, ah
            xor ah, ah
            push ax
                ; calculate entered password
                mov cl, (FO_PW_LEN_MAX + 1)
                mul cl
                mov si, fo_entered_passwords_list
                add si, ax

                ; calculate real password
                cmp byte [ds:fo_pw_level], 4
                jnz .check_password_not_4
                    mov di, fo_pw_list_4
                    jmp .check_password_di_correct
                .check_password_not_4:
                cmp byte [ds:fo_pw_level], 3
                jnz .check_password_not_3
                    mov di, fo_pw_list_3
                    jmp .check_password_di_correct
                .check_password_not_3:
                cmp byte [ds:fo_pw_level], 2
                jnz .check_password_not_2
                    mov di, fo_pw_list_2
                    jmp .check_password_di_correct
                .check_password_not_2:
                cmp byte [ds:fo_pw_level], 1
                jnz .check_password_not_1
                    mov di, fo_pw_list_1
                    jmp .check_password_di_correct
                .check_password_not_1:

                    mov di, fo_pw_list_0
                .check_password_di_correct:
                
                xor ax, ax
                xor bx, bx
                mov al, [ds:fo_random_pw_id]
                mov bl, [ds:fo_pw_level]
                mov cl, [ds:fo_pw_length + bx]
                mul cl
                add di, ax

                ; calculate number of characters
                xor bx, bx
                xor cx, cx
                mov bl, [ds:fo_pw_level]
                mov cl, [ds:fo_pw_length + bx]

                push si
                push di
                    repz cmpsb
                pop di
                pop si
            pop ax
            jz .check_password_correct
            jmp .analyze_correct_chars
        .check_password_correct:
            clc
            popa
            ret

    ; Writes the likeliness to the variable
    ; input: si -> entered password offset
    ;        di -> correct password
    ;        ax -> try-number
    .analyze_correct_chars:
        ;pusha -> pusha in .check_password
        push bx

        xor bx, bx
        mov bx, fo_entered_passwords_matches_list
        add bx, ax ; bx points to likeliness variable

        .analyze_correct_chars_loop:
            cmp byte [ds:si], 0
            jz .analyze_correct_chars_done
            cmpsb
            jnz .analyze_correct_chars_loop ; not equal

            ; equal, increase likeliness
            inc byte [ds:bx]
            jmp .analyze_correct_chars_loop ; next char        

        .analyze_correct_chars_done:
            pop bx
            popa
            stc
            ret

    .build_level:
        mov ax, [ds:fo_no_of_fill_chars_lv_current]
        mov word [ds:fo_fill_characters_left], ax ; FO_NO_OF_FILL_CHARS_LV_1
        xor ax, ax
        mov bp, ax ;password identifier

        mov cx, FO_NO_OF_PWS
        .fill_dsp_var:
            ; calculate number of fill chars to display
            xor ax, ax
            int 0xf4 ; get random byte into al
            and al, [ds:fo_max_fill_chars_lv_current] ; FO_MAX_FILL_CHARS_LV_1 ; maximum is FO_MAX_FILL_CHARS_LV_1
            ; al is between 0 and FO_MAX_FILL_CHARS_LV_1
            cmp word [ds:fo_fill_characters_left], ax
            jg .skip_adjust_fill_chars_ax
                mov ax, word [ds:fo_fill_characters_left]
            .skip_adjust_fill_chars_ax:
            sub word [ds:fo_fill_characters_left], ax ; remaining fill chars after this round
            mov [ds:fo_no_of_fill_chars_current_round], al

            ; write first half of fill chars to var
            mov al, [ds:fo_no_of_fill_chars_current_round]
            shr al, 1 ; display one half before the password, the other half after the password
            inc al ; display at least 1 character
            push cx
                xor cx, cx
                mov cl, al
                .fill_char_pre_loop:
                    call .get_random_obfuscator_character
                    mov [es:di], al
                    inc di
                    loop .fill_char_pre_loop
            pop cx

            ; select real/fake password
            mov si, [ds:fo_pw_list_offset_current] ; fo_pw_list_1
            cmp byte [ds:fo_real_pw_written], FALSE
            jnz .fill_rand_pw

            cmp cx, 1
            jz .fill_real_pw ; last chance for real password

            int 0xf4 ; get random byte into al
            test al, 1
            jz .fill_rand_pw

            ; the real password it is...
            .fill_real_pw:
            mov byte [ds:fo_real_pw_written], 1
            push cx
                ; calculate offset of pw in list
                xor ax, ax
                xor cx, cx
                mov al, [ds:fo_random_pw_id]
                mov cl, [ds:fo_pw_len_lv_current] ; FO_PW_LEN_LV_1
                mul cl
                add si, ax
            pop cx
            jmp .fill_pw_from_si

            .fill_rand_pw:
            push cx
                ; calculate offset of pw in list
                xor ax, ax
                xor cx, cx
                mov al, [ds:fo_pw_list_used_pw_ids + bp]
                mov cl, [ds:fo_pw_len_lv_current] ; FO_PW_LEN_LV_1
                mul cl
                add si, ax
            pop cx
            inc bp

            ; write password to display var
            .fill_pw_from_si:
            push cx
                xor cx, cx
                mov cl, [ds:fo_pw_len_lv_current] ; FO_PW_LEN_LV_1
                cld
                rep movsb
            pop cx

            ; write second half of fill chars to var
            mov ah, [ds:fo_no_of_fill_chars_current_round]
            mov al, ah
            shr ah, 1 
            sub al, ah ; calculate the rest of the fill chars to display
            inc al ; display at least 1 character
            push cx
                xor cx, cx
                mov cl, al
                .fill_char_post_loop:
                    call .get_random_obfuscator_character
                    mov [es:di], al
                    inc di
                    loop .fill_char_post_loop
            pop cx

        dec cx
        cmp cx, 0
        jnz .fill_dsp_var

        ; Fill rest of the screen
        mov cx, [ds:fo_fill_characters_left]
        cmp cx, 0
        jz .fill_dsp_to_end_finished
        .fill_dsp_to_end:
            call .get_random_obfuscator_character
            mov [es:di], al
            inc di
        loop .fill_dsp_to_end
        .fill_dsp_to_end_finished:    

        ret

    ; Build structure (hex prefix) and increase hex offset
    .display_hex_prefix:
        pusha
            mov al, ASCII_0
            call fo_putch
            mov al, ASCII_X_LOWER
            call fo_putch
            mov dl, [ds:fo_hex_offset + 1]
            int 0xe4
            mov al, dh
            call fo_putch
            mov al, dl
            call fo_putch
            mov dl, [ds:fo_hex_offset]
            int 0xe4
            mov al, dh
            call fo_putch
            mov al, dl
            call fo_putch
            mov al, ASCII_SP
            call fo_putch
            mov ax, FO_CHARS_PER_LINE
            add [ds:fo_hex_offset], ax
        popa
        ret

    ; returns random obfuscator character into al
    .get_random_obfuscator_character:
        push si
        xor ax, ax

        int 0xf4 ; get random byte into al
        and al, 0b11111 ; maximum is 32
        cmp al, FO_OBFUSCATOR_CHARS_COUNT
        jb .display_random_obfuscator_character_skip_adj_al
            sub al, FO_OBFUSCATOR_CHARS_COUNT
        .display_random_obfuscator_character_skip_adj_al:
        ; al is between 0 and FO_OBFUSCATOR_CHARS_COUNT

        mov si, fo_obfuscator_char_list
        add si, ax
        mov al, [ds:si]

        pop si
        ret


    .pw_exists:
        pop cx
        jmp .loop_create_fake_pws

    .login:
        mov bx, FO_USER_PROMPT_X
        mov dx, FO_USER_PROMPT_Y
        mov ah, 1
        int 0x99 ; set cursors

        mov al, [ds:fo_input_char]
        call fo_putch

        mov si, fo3_success
        call fo_display_line_no_ln

        mov ax, 1500
        int 0x81 ; sleep ax milli seconds

        jmp .done

    .done:
        ret

    .y_org dw 0
    .x_org dw 0
    .pw_list_index db 0

; putch the character in al with delay
fo_putch:
    push ax
        xor ax, ax
        int 0xf4 ; get random byte into al
        cmp byte [cs:fo_fast_mode], FALSE
        jz .slow
            xor ax, ax ; fast mode -> wait 2 ms
        .slow:
        and al, 0b1111 ; maximum of 15 ms
        add al, 2 ; wait at least 2 ms
        ; => ax contains random number between 2 and 17

        push ax
            mov al, [cs:fo_use_sound]
            cmp al, TRUE
            jnz .skip_sound
                mov ax, NOTE_C2
                int 0x88
            .skip_sound:
        pop ax

        int 0x81 ; sleep ax milli seconds
    pop ax

    int 0x90 ; display character

    push ax
        int 0x89
    pop ax

    ret

; reads keyboard intput for cx bytes and stores it to es:di
; does not invoke a linebreak
fo_read_no_linebreak:
    pusha

    dec cx ; terminator

    mov dx, di
    mov bx, cx ; Remember maximum size

    .loop:
        mov ah, 0xff
        int 0xb0 ; getch no putch

        cmp al, 0x08 ; Backspace
        je .backspace

        cmp al, 0x0d ; Return
        je .return

        cmp cx, 0 ; No more bytes -> Only accept backspace and return
        je .loop

        call fo_putch
        stosb ; Save to di
        dec cx
        jmp .loop ; Give the user the chance to enter bs and cr

    .backspace:
        cmp bx, cx
        je .loop ; Beginning of input, do not delete data at a position befor di

        dec di
        mov byte [di], 0 ; Delete backspace character
        inc cx ; A new byte is free for data

        mov al, ASCII_BS
        call fo_putch

        jmp .loop

    .return:
        int 0xb0 ; getch lf remaining in buffer

        ; null-terminate the string 
        mov al, 0
        stosb

        popa
    ret
;-------------------------------------------------------------------------------