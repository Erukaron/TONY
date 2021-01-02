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
; Common definitions for various files
;-------------------------------------------------------------------------------
%ifndef __COMMON_INC__INCLUDED__
%define __COMMON_INC__INCLUDED__
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Segments
;-------------------------------------------------------------------------------
INTERRUPT_SEGMENT           equ 0x0000
INTERRUPT_OFFSET            equ 0x0000
ROOT_SEGMENT                equ 0x0000
ROOT_OFFSET                 equ 0x0600
FAT_SEGMENT                 equ 0x0000
FAT_OFFSET                  equ 0x2200
BOOT_SEGMENT                equ 0x07c0
BOOT_OFFSET                 equ 0x0000
KERNEL_SEGMENT              equ 0x0000
KERNEL_OFFSET               equ 0x3400
STACK_SEGMENT               equ 0x1000
STACK_OFFSET                equ 0x0000
COLORDISPLAY_TEXT_SEGMENT   equ 0xb800
COLORDISPLAY_TEXT_OFFSET    equ 0x0000
COLORDISPLAY_GFX_SEGMENT    equ 0xa000
COLORDISPLAY_GFX_OFFSET     equ 0x0000
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; BIOS modes
;-------------------------------------------------------------------------------
VIDEOMODE_BIOS_ASCII_COLOR_80x25 equ 0x03
VIDEOMODE_BIOS_GFX_320_200_COLOR equ 0x13
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Video mode
;-------------------------------------------------------------------------------
VIDEOMODE_ASCII_COLOR_80x25 equ 0
VIDEOMODE_GFX_320_200_COLOR equ 1
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Miscellanious
;-------------------------------------------------------------------------------
FALSE equ 0
TRUE  equ 1
EOF   equ 1
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; ASCII
;-------------------------------------------------------------------------------
BACKSPACE  equ 0x08
LINEFEED   equ 0x0a
RETURN     equ 0x0d
SPACE      equ 0x20
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; COM
;-------------------------------------------------------------------------------
PSP_LENGTH                  equ 0x100
PSP_CMD_LINE_OFFSET         equ 0x81
PSP_CMD_LINE_MAX_LENGTH     equ 0x7e ; (0xff - 0x81)
PSP_CMD_LINE_LENGTH_OFFSET  equ 0x80
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%endif ;__COMMON_INC__INCLUDED__
;-------------------------------------------------------------------------------