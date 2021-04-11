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
INTERRUPT_SEGMENT               equ 0x0000
INTERRUPT_OFFSET                equ 0x0000
ROOT_SEGMENT                    equ 0x0000
ROOT_OFFSET                     equ 0x0600
FAT_SEGMENT                     equ 0x0000
FAT_OFFSET                      equ 0x2200
BOOT_SEGMENT                    equ 0x07c0
BOOT_OFFSET                     equ 0x0000
KERNEL_SEGMENT                  equ 0x0000
KERNEL_OFFSET                   equ 0x3400
STACK_SEGMENT                   equ 0x1000
STACK_OFFSET                    equ 0x0000
BOOT_COPY_SEGMENT               equ 0x2000
BOOT_COPY_OFFSET                equ 0x0000
GRAYSCALEDISPLAY_TEXT_SEGMENT   equ 0xb800
GRAYSCALEDISPLAY_TEXT_OFFSET    equ 0x0000
COLORDISPLAY_TEXT_SEGMENT       equ 0xb800
COLORDISPLAY_TEXT_OFFSET        equ 0x0000
COLORDISPLAY_GFX_SEGMENT        equ 0xa000
COLORDISPLAY_GFX_OFFSET         equ 0x0000
VIDEO_DOUBLE_BUFFER_SEGMENT     equ 0x8fc0
VIDEO_DOUBLE_BUFFER_OFFSET      equ 0x0000
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; BIOS modes
;-------------------------------------------------------------------------------
VIDEOMODE_BIOS_ASCII_GRAY_40x25  equ 0x00
VIDEOMODE_BIOS_ASCII_COLOR_80x25 equ 0x03
VIDEOMODE_BIOS_GFX_320_200_COLOR equ 0x13
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; Video mode
;  40x 25 text : Bit 5 set
;  80x 25 text : Bit 6 set
; 320x200 gfx  : Bit 7 set
; Color mode: Bit 0 - Bit 1
; Monochrome   : 00
; Grayscale    : 01
; Color        : 10
;-------------------------------------------------------------------------------
VIDEO_MODE_40x25            equ 0b0010000
VIDEO_MODE_80x25            equ 0b0100000
VIDEO_MODE_GFX_320x200      equ 0b1000000
VIDEOMODE_ASCII_GRAY_40x25  equ 0b0010001
VIDEOMODE_ASCII_COLOR_80x25 equ 0b0100010
VIDEOMODE_GFX_320_200_COLOR equ 0b1000010
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
; Notes
;-------------------------------------------------------------------------------
; Note  Frequency   Frequency #
; C     130.81      9121
; C#    138.59      8609
; D     146.83      8126
; D#    155.56      7670
; E     164.81      7239
; F     174.61      6833
; F#    185.00      6449
; G     196.00      6087
; G#    207.65      5746
; A     220.00      5423
; A#    233.08      5119
; B     246.94      4831
; Middle C  261.63  4560
; C#    277.18      4304
; D     293.66      4063
; D#    311.13      3834
; E     329.63      3619
; F     349.23      3416
; F#    369.99      3224
; G     391.00      3043
; G#    415.30      2873
; A     440.00      2711
; A#    466.16      2559
; B     493.88      2415
; C     523.25      2280
; C#    554.37      2152
; D     587.33      2031
; D#    622.25      1917
; E     659.26      1809
; F     698.46      1715
; F#    739.99      1612
; G     783.99      1521
; G#    830.61      1436
; A     880.00      1355
; A#    923.33      1292
; B     987.77      1207
; C     1046.50     1140
NOTE_C0         equ 9121
NOTE_CIS0       equ 8609
NOTE_D0         equ 8126
NOTE_DIS0       equ 7670
NOTE_E0         equ 7239
NOTE_F0         equ 6833
NOTE_FIS0       equ 6449
NOTE_G0         equ 6087
NOTE_GIS0       equ 5746
NOTE_A0         equ 5423
NOTE_AIS0       equ 5119
NOTE_B0         equ 4831
NOTE_C1         equ 4560
NOTE_CIS1       equ 4304
NOTE_D1         equ 4063
NOTE_DIS1       equ 3834
NOTE_E1         equ 3619
NOTE_F1         equ 3416
NOTE_FIS1       equ 3224
NOTE_G1         equ 3043
NOTE_GIS1       equ 2873
NOTE_A1         equ 2711
NOTE_AIS1       equ 2559
NOTE_B1         equ 2415
NOTE_C2         equ 2280
NOTE_CIS2       equ 2152
NOTE_D2         equ 2031
NOTE_DIS2       equ 1917
NOTE_E2         equ 1809
NOTE_F2         equ 1715
NOTE_FIS2       equ 1612
NOTE_G2         equ 1521
NOTE_GIS2       equ 1436
NOTE_A2         equ 1355
NOTE_AIS2       equ 1292
NOTE_B2         equ 1207
NOTE_C3         equ 1140
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
%endif ;__COMMON_INC__INCLUDED__
;-------------------------------------------------------------------------------