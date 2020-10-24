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
; TONY Keyboard map layout file (german)
;-------------------------------------------------------------------------------
bits 16
org 0

%include "SCANCODE.INC"
%include "FUNCCODE.INC"
%include "ASCII.INC"
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
table_start_addr  dw table_start
table_end_addr    dw table_end
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Table organization:
; Scan code (16 bit)  ASCII representation (lower)       ASCII representation (UPPER)         ASCII representation (altgr)
; Each entry is 5 bytes
;
; If 16 bit ASCII representation (lower), ASCII representation (UPPER) equals 0xBEEF, then ASCII representation (altgr) contains a function key code!
;-------------------------------------------------------------------------------
table_start:

	; function keys row
	db                           0, SCAN_CODE_ESCAPE        , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_ESC
	db                           0, SCAN_CODE_F1            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F1
	db                           0, SCAN_CODE_F2            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F2
	db                           0, SCAN_CODE_F3            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F3
	db                           0, SCAN_CODE_F4            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F4
	db                           0, SCAN_CODE_F5            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F5
	db                           0, SCAN_CODE_F6            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F6
	db                           0, SCAN_CODE_F7            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F7
	db                           0, SCAN_CODE_F8            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F8
	db                           0, SCAN_CODE_F9            , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F9
	db                           0, SCAN_CODE_F10           , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F10
	db                           0, SCAN_CODE_F11           , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F11
	db                           0, SCAN_CODE_F12           , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_F12
	db                           0, SCAN_CODE_PRINT_SCREEN  , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_PRINT_SCREEN
	db                           0, SCAN_CODE_SCROLL_LOCK   , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_SCROLL_LOCK
	 
	; group of six above arrow keys
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_INSERT        , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_INSERT
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_DELETE        , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_DELETE
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_HOME          , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_HOME
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_END           , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_END
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_PAGE_UP       , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_PAGE_UP
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_PAGE_DOWN     , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_PAGE_DOWN
	 
	; arrow keys
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_DOWN_ARROW    , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_DOWN_ARROW
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_RIGHT_ARROW   , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_RIGHT_ARROW
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_LEFT_ARROW    , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_LEFT_ARROW
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_UP_ARROW      , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_UP_ARROW
	 
	; first row
	db                           0, SCAN_CODE_BACKQUOTE     , ASCII_ZIRKUMFLEX  , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_1      , ASCII_1           , ASCII_EXCLAMATION        , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_2      , ASCII_2           , ASCII_QUOTATION          , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_3      , ASCII_3           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_4      , ASCII_4           , ASCII_DOLLAR             , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_5      , ASCII_5           , ASCII_PERCENT            , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_6      , ASCII_6           , ASCII_AMPERSAND          , ASCII_NUL
	db                           0, SCAN_CODE_NUMBER_7      , ASCII_7           , ASCII_SLASH              , ASCII_CURVED_BRACKET_OPEN
	db                           0, SCAN_CODE_NUMBER_8      , ASCII_8           , ASCII_PARENTHESIS_OPEN   , ASCII_BRACKET_OPEN
	db                           0, SCAN_CODE_NUMBER_9      , ASCII_9           , ASCII_PARENTHESIS_CLOSE  , ASCII_BRACKET_CLOSE
	db                           0, SCAN_CODE_NUMBER_0      , ASCII_0           , ASCII_EQUALS             , ASCII_CURVED_BRACKET_CLOSE
	db                           0, SCAN_CODE_MINUS         , ASCII_NUL         , ASCII_QUESTION           , ASCII_BACKSLASH
	db                           0, SCAN_CODE_EQUALS        , ASCII_APOSTROPHE  , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_BACKSPACE     , ASCII_BS          , ASCII_NUL                , ASCII_NUL
	 
	; second row
	db                           0, SCAN_CODE_TAB           , ASCII_TAB         , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_Q             , ASCII_Q_LOWER     , ASCII_Q                  , ASCII_AT
	db                           0, SCAN_CODE_W             , ASCII_W_LOWER     , ASCII_W                  , ASCII_NUL
	db                           0, SCAN_CODE_E             , ASCII_E_LOWER     , ASCII_E                  , ASCII_NUL
	db                           0, SCAN_CODE_R             , ASCII_R_LOWER     , ASCII_R                  , ASCII_NUL
	db                           0, SCAN_CODE_T             , ASCII_T_LOWER     , ASCII_T                  , ASCII_NUL
	db                           0, SCAN_CODE_Y             , ASCII_Z_LOWER     , ASCII_Z                  , ASCII_NUL
	db                           0, SCAN_CODE_U             , ASCII_U_LOWER     , ASCII_U                  , ASCII_NUL
	db                           0, SCAN_CODE_I             , ASCII_I_LOWER     , ASCII_I                  , ASCII_NUL
	db                           0, SCAN_CODE_O             , ASCII_O_LOWER     , ASCII_O                  , ASCII_NUL
	db                           0, SCAN_CODE_P             , ASCII_P_LOWER     , ASCII_P                  , ASCII_NUL
	;db SCAN_CODE_SQR_BRACKET_L , 
	db                           0, SCAN_CODE_SQR_BRACKET_R , ASCII_PLUS        , ASCII_ASTERIX            , ASCII_TILT
	;db SCAN_CODE_SQR_BACKSLASH , 
	
	; third row
	db                           0, SCAN_CODE_CAPS_LOCK     , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_CAPS_LOCK  
	db                           0, SCAN_CODE_A             , ASCII_A_LOWER     , ASCII_A                  , ASCII_NUL
	db                           0, SCAN_CODE_S             , ASCII_S_LOWER     , ASCII_S                  , ASCII_NUL
	db                           0, SCAN_CODE_D             , ASCII_D_LOWER     , ASCII_D                  , ASCII_NUL
	db                           0, SCAN_CODE_F             , ASCII_F_LOWER     , ASCII_F                  , ASCII_NUL
	db                           0, SCAN_CODE_G             , ASCII_G_LOWER     , ASCII_G                  , ASCII_NUL
	db                           0, SCAN_CODE_H             , ASCII_H_LOWER     , ASCII_H                  , ASCII_NUL
	db                           0, SCAN_CODE_J             , ASCII_J_LOWER     , ASCII_J                  , ASCII_NUL
	db                           0, SCAN_CODE_K             , ASCII_K_LOWER     , ASCII_K                  , ASCII_NUL
	db                           0, SCAN_CODE_L             , ASCII_L_LOWER     , ASCII_L                  , ASCII_NUL
	;db SCAN_CODE_SEMICOLON     , 
	db                           0, SCAN_CODE_GER_HASHTAG   , ASCII_HASHTAG     , ASCII_SINGLE_QUOTATION   , ASCII_NUL
	db                           0, SCAN_CODE_ENTER         , ASCII_CR          , ASCII_NUL                , ASCII_NUL
	
	; fourth row
	db                           0, SCAN_CODE_LEFT_SHIFT    , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_SHIFT    
	db                           0, SCAN_CODE_GER_LESSER	, ASCII_SMALLER     , ASCII_GREATER            , ASCII_PIPE
	db                           0, SCAN_CODE_Z             , ASCII_Y_LOWER     , ASCII_Y                  , ASCII_NUL
	db                           0, SCAN_CODE_X             , ASCII_X_LOWER     , ASCII_X                  , ASCII_NUL
	db                           0, SCAN_CODE_C             , ASCII_C_LOWER     , ASCII_C                  , ASCII_NUL
	db                           0, SCAN_CODE_V             , ASCII_V_LOWER     , ASCII_V                  , ASCII_NUL
	db                           0, SCAN_CODE_B             , ASCII_B_LOWER     , ASCII_B                  , ASCII_NUL
	db                           0, SCAN_CODE_N             , ASCII_N_LOWER     , ASCII_N                  , ASCII_NUL
	db                           0, SCAN_CODE_M             , ASCII_M_LOWER     , ASCII_M                  , ASCII_NUL
	db                           0, SCAN_CODE_COMMA         , ASCII_COMMA       , ASCII_SEMICOLON          , ASCII_NUL
	db                           0, SCAN_CODE_PERIOD        , ASCII_DOT         , ASCII_COLUMN             , ASCII_NUL
	db                           0, SCAN_CODE_SLASH         , ASCII_MINUS       , ASCII_UNDERSCORE         , ASCII_NUL
	db                           0, SCAN_CODE_RIGHT_SHIFT   , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_SHIFT_R    
	
	; fifth row
	db                           0, SCAN_CODE_LEFT_CONTROL  , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_CTRL 
	db                           0, SCAN_CODE_LEFT_ALT      , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_ALT    
	db                           0, SCAN_CODE_SPACE_BAR     , ASCII_SP          , ASCII_NUL                , ASCII_NUL
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_RIGHT_ALT     , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_ALT_GR 
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_RIGHT_CONTROL , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_CTRL_R 
	
	; keypad
	db                           0, SCAN_CODE_NUMLOCK       , FUNC_KEY_IDENT_A  , FUNC_KEY_IDENT_B         , FUNC_KEY_NUM_LOCK
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_KEYPAD_SLASH  , ASCII_SLASH       , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_ASTRISK, ASCII_ASTERIX     , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_MINUS  , ASCII_MINUS       , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_7      , ASCII_7           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_8      , ASCII_8           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_9      , ASCII_9           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_4      , ASCII_4           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_5      , ASCII_5           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_6      , ASCII_6           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_PLUS   , ASCII_PLUS        , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_1      , ASCII_1           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_2      , ASCII_2           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_3      , ASCII_3           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_0      , ASCII_0           , ASCII_NUL                , ASCII_NUL
	db                           0, SCAN_CODE_KEYPAD_PERIOD , ASCII_COMMA       , ASCII_NUL                , ASCII_NUL
	db KEYBOARD_EXTENDED_SCAN_CODE, SCAN_CODE_KEYPAD_ENTER  , ASCII_CR          , ASCII_NUL                , ASCII_NUL


table_end:
;-------------------------------------------------------------------------------