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
; TONY ascii constants
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
ASCII_NUL                  equ 0x00
ASCII_BS                   equ 0x08
ASCII_TAB                  equ 0x09
ASCII_LF                   equ 0x0a
ASCII_CR                   equ 0x0d
ASCII_CRLF                 equ 0x0a0d
ASCII_LFCR                 equ 0x0d0a
ASCII_ESC                  equ 0x1b

ASCII_FIRST_PRINTABLE_CHAR equ 0x20
ASCII_SP                   equ 0x20
ASCII_EXCLAMATION          equ 0x21
ASCII_QUOTATION            equ 0x22
ASCII_HASHTAG              equ 0x23
ASCII_DOLLAR               equ 0x24
ASCII_PERCENT              equ 0x25
ASCII_AMPERSAND            equ 0x26
ASCII_SINGLE_QUOTATION     equ 0x27
ASCII_PARENTHESIS_OPEN     equ 0x28
ASCII_PARENTHESIS_CLOSE    equ 0x29
ASCII_ASTERIX              equ 0x2a
ASCII_PLUS                 equ 0x2b
ASCII_COMMA                equ 0x2c
ASCII_MINUS                equ 0x2d
ASCII_DOT                  equ 0x2e
ASCII_SLASH                equ 0x2f
ASCII_0                    equ 0x30
ASCII_1                    equ 0x31
ASCII_2                    equ 0x32
ASCII_3                    equ 0x33
ASCII_4                    equ 0x34
ASCII_5                    equ 0x35
ASCII_6                    equ 0x36
ASCII_7                    equ 0x37
ASCII_8                    equ 0x38
ASCII_9                    equ 0x39
ASCII_COLUMN               equ 0x3a
ASCII_SEMICOLON            equ 0x3b
ASCII_SMALLER              equ 0x3c
ASCII_EQUALS               equ 0x3d
ASCII_GREATER              equ 0x3e
ASCII_QUESTION             equ 0x3f
ASCII_AT                   equ 0x40
ASCII_A                    equ 0x41
ASCII_B                    equ 0x42
ASCII_C                    equ 0x43
ASCII_D                    equ 0x44
ASCII_E                    equ 0x45
ASCII_F                    equ 0x46
ASCII_G                    equ 0x47
ASCII_H                    equ 0x48
ASCII_I                    equ 0x49
ASCII_J                    equ 0x4a
ASCII_K                    equ 0x4b
ASCII_L                    equ 0x4c
ASCII_M                    equ 0x4d
ASCII_N                    equ 0x4e
ASCII_O                    equ 0x4f
ASCII_P                    equ 0x50
ASCII_Q                    equ 0x51
ASCII_R                    equ 0x52
ASCII_S                    equ 0x53
ASCII_T                    equ 0x54
ASCII_U                    equ 0x55
ASCII_V                    equ 0x56
ASCII_W                    equ 0x57
ASCII_X                    equ 0x58
ASCII_Y                    equ 0x59
ASCII_Z                    equ 0x5a
ASCII_BRACKET_OPEN         equ 0x5b
ASCII_BACKSLASH            equ 0x5c
ASCII_BRACKET_CLOSE        equ 0x5d
ASCII_ZIRKUMFLEX           equ 0x5e
ASCII_UNDERSCORE           equ 0x5f
ASCII_APOSTROPHE           equ 0x60
ASCII_A_LOWER              equ 0x61
ASCII_B_LOWER              equ 0x62
ASCII_C_LOWER              equ 0x63
ASCII_D_LOWER              equ 0x64
ASCII_E_LOWER              equ 0x65
ASCII_F_LOWER              equ 0x66
ASCII_G_LOWER              equ 0x67
ASCII_H_LOWER              equ 0x68
ASCII_I_LOWER              equ 0x69
ASCII_J_LOWER              equ 0x6a
ASCII_K_LOWER              equ 0x6b
ASCII_L_LOWER              equ 0x6c
ASCII_M_LOWER              equ 0x6d
ASCII_N_LOWER              equ 0x6e
ASCII_O_LOWER              equ 0x6f
ASCII_P_LOWER              equ 0x70
ASCII_Q_LOWER              equ 0x71
ASCII_R_LOWER              equ 0x72
ASCII_S_LOWER              equ 0x73
ASCII_T_LOWER              equ 0x74
ASCII_U_LOWER              equ 0x75
ASCII_V_LOWER              equ 0x76
ASCII_W_LOWER              equ 0x77
ASCII_X_LOWER              equ 0x78
ASCII_Y_LOWER              equ 0x79
ASCII_Z_LOWER              equ 0x7a
ASCII_CURVED_BRACKET_OPEN  equ 0x7b
ASCII_PIPE                 equ 0x7c
ASCII_CURVED_BRACKET_CLOSE equ 0x7d
ASCII_TILT                 equ 0x7e
ASCII_LAST_PRINTABLE_CHAR  equ 0x7e
;-------------------------------------------------------------------------------