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
// COM header file
//-------------------------------------------------------------------------------
#ifndef __COM_H__INCLUDED__
#define __COM_H__INCLUDED__
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
#include "COMMON.h"
asm("org PSP_LENGTH");
asm("jmp __com_prg_init__");
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Common com-variables
//-----------------------------------------------------------------------------
asm("argc db 0");
asm("argv times 0x80 + 1 db 0");
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Set everything up
//-----------------------------------------------------------------------------
asm("__com_prg_init__:");
asm("    xor cx, cx");
asm("    mov cl, [cs:PSP_CMD_LINE_LENGTH_OFFSET]");
asm("    mov byte [ds:argc], cl");
asm("    mov si, PSP_CMD_LINE_OFFSET");
asm("    xor bx, bx");
asm("    mov dx, cx");
asm("    mov di, argv");
asm("    int 0xd3"); // Copy psp command line to argv

    // argv and argc are parameters
asm("    push word argv");
asm("    xor ax, ax");
asm("    mov al, [ds:argc]");
asm("    push ax");
    // Proceed with execution of com file
asm("    call _main");
    // Alignment for argv and argc passed via stack
asm("    sub sp, -4");
    // Exit via return to exit interrupt
asm("    ret");
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------------
#endif //__COM_H__INCLUDED__
//-------------------------------------------------------------------------------