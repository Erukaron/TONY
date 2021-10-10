//-------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2020 Erukaron
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Common definitions for various files
//-------------------------------------------------------------------------------
#ifndef __COMMON_H__INCLUDED__
#define __COMMON_H__INCLUDED__
asm("%include \"COMMON.INC\"");
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Segments
//-------------------------------------------------------------------------------
#define INTERRUPT_SEGMENT                0x0000
#define INTERRUPT_OFFSET                 0x0000
#define ROOT_SEGMENT                     0x0000
#define ROOT_OFFSET                      0x0600
#define FAT_SEGMENT                      0x0000
#define FAT_OFFSET                       0x2200
#define BOOT_SEGMENT                     0x07c0
#define BOOT_OFFSET                      0x0000
#define KERNEL_SEGMENT                   0x0000
#define KERNEL_OFFSET                    0x3400
#define STACK_SEGMENT                    0x1000
#define STACK_OFFSET                     0x0000
#define BOOT_COPY_SEGMENT                0x2000
#define BOOT_COPY_OFFSET                 0x0000
#define GRAYSCALEDISPLAY_TEXT_SEGMENT    0xb800
#define GRAYSCALEDISPLAY_TEXT_OFFSET     0x0000
#define COLORDISPLAY_TEXT_SEGMENT        0xb800
#define COLORDISPLAY_TEXT_OFFSET         0x0000
#define COLORDISPLAY_GFX_SEGMENT         0xa000
#define COLORDISPLAY_GFX_OFFSET          0x0000
#define VIDEO_DOUBLE_BUFFER_SEGMENT      0x8fc0
#define VIDEO_DOUBLE_BUFFER_OFFSET       0x0000
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// BIOS modes
//-------------------------------------------------------------------------------
#define VIDEOMODE_BIOS_ASCII_GRAY_40x25   0x00
#define VIDEOMODE_BIOS_ASCII_COLOR_80x25  0x03
#define VIDEOMODE_BIOS_GFX_320_200_COLOR  0x13
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Video mode
//  40x 25 text : Bit 5 set
//  80x 25 text : Bit 6 set
// 320x200 gfx  : Bit 7 set
// Color mode: Bit 0 - Bit 1
// Monochrome   : 00
// Grayscale    : 01
// Color        : 10
//-------------------------------------------------------------------------------
#define VIDEO_MODE_40x25             0b0010000
#define VIDEO_MODE_80x25             0b0100000
#define VIDEO_MODE_GFX_320x200       0b1000000
#define VIDEOMODE_ASCII_GRAY_40x25   0b0010001
#define VIDEOMODE_ASCII_COLOR_80x25  0b0100010
#define VIDEOMODE_GFX_320_200_COLOR  0b1000010
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Miscellanious
//-------------------------------------------------------------------------------
#define FALSE  0
#define TRUE   ~FALSE
#define EOF    1
#define DBG    asm("xchg bx, bx");
#define dbg    asm("xchg bx, bx");
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// ASCII
//-------------------------------------------------------------------------------
#define BACKSPACE   0x08
#define LINEFEED    0x0a
#define RETURN      0x0d
#define SPACE       0x20
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// COM
//-------------------------------------------------------------------------------
#define PSP_LENGTH                   0x100
#define PSP_CMD_LINE_OFFSET          0x81
#define PSP_CMD_LINE_MAX_LENGTH      0x7e // (0xff - 0x81)
#define PSP_CMD_LINE_LENGTH_OFFSET   0x80
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Notes
//-------------------------------------------------------------------------------
// Note  Frency   Frency #
// C     130.81      9121
// C#    138.59      8609
// D     146.83      8126
// D#    155.56      7670
// E     164.81      7239
// F     174.61      6833
// F#    185.00      6449
// G     196.00      6087
// G#    207.65      5746
// A     220.00      5423
// A#    233.08      5119
// B     246.94      4831
// Middle C  261.63  4560
// C#    277.18      4304
// D     293.66      4063
// D#    311.13      3834
// E     329.63      3619
// F     349.23      3416
// F#    369.99      3224
// G     391.00      3043
// G#    415.30      2873
// A     440.00      2711
// A#    466.16      2559
// B     493.88      2415
// C     523.25      2280
// C#    554.37      2152
// D     587.33      2031
// D#    622.25      1917
// E     659.26      1809
// F     698.46      1715
// F#    739.99      1612
// G     783.99      1521
// G#    830.61      1436
// A     880.00      1355
// A#    923.33      1292
// B     987.77      1207
// C     1046.50     1140
#define NOTE_C0          9121
#define NOTE_CIS0        8609
#define NOTE_D0          8126
#define NOTE_DIS0        7670
#define NOTE_E0          7239
#define NOTE_F0          6833
#define NOTE_FIS0        6449
#define NOTE_G0          6087
#define NOTE_GIS0        5746
#define NOTE_A0          5423
#define NOTE_AIS0        5119
#define NOTE_B0          4831
#define NOTE_C1          4560
#define NOTE_CIS1        4304
#define NOTE_D1          4063
#define NOTE_DIS1        3834
#define NOTE_E1          3619
#define NOTE_F1          3416
#define NOTE_FIS1        3224
#define NOTE_G1          3043
#define NOTE_GIS1        2873
#define NOTE_A1          2711
#define NOTE_AIS1        2559
#define NOTE_B1          2415
#define NOTE_C2          2280
#define NOTE_CIS2        2152
#define NOTE_D2          2031
#define NOTE_DIS2        1917
#define NOTE_E2          1809
#define NOTE_F2          1715
#define NOTE_FIS2        1612
#define NOTE_G2          1521
#define NOTE_GIS2        1436
#define NOTE_A2          1355
#define NOTE_AIS2        1292
#define NOTE_B2          1207
#define NOTE_C3          1140
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
#endif //__COMMON_H__INCLUDED__
//-------------------------------------------------------------------------------