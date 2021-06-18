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
// Standard input output library
//-------------------------------------------------------------------------------
#ifndef __STDIO_H_INCLUDED
#define __STDIO_H_INCLUDED
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Typedefs
//-------------------------------------------------------------------------------
typedef char byte;
typedef short word;
typedef struct 
{
    short segment;
    byte offset;
    short size;
} reserved_memory;
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Prototypes
//-------------------------------------------------------------------------------
// int 0x90
void putch(char c);
// int 0x91
void printf(char *str);
// int 0x92
void printf_loc(char *str, char y, char x);

// int 0xb0
char getch();
char getch_no_putch();
// int 0xb1
void readln(int length, char *buffer);

// int 0xb8
reserved_memory malloc(short size);
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Functions
// Local variables:
//      [ss:bp-2] is first local variable
//      [ss:bp-4] is second local variable...
// Parameters:
//      [ss:bp+4] is first parameter
//      [ss:bp+6] is second parameter...
//-------------------------------------------------------------------------------
void putch(char c)
{
    asm("mov al, [ss:bp+4]");
    asm("int 0x90");
}

void printf(char *str)
{
    asm("mov si, [ss:bp+4]");
    asm("int 0x91");
}

void printf_loc(char *str, char y, char x)
{
    asm("mov si, [ss:bp+4]");
    asm("mov ax, [ss:bp+6]");
    asm("mov bx, [ss:bp+8]");
    asm("mov ah, al");
    asm("mov al, bl");
    asm("int 0x92");
}

char getch()
{
    asm("mov ah, 0");
    asm("int 0xb0");
}
char getch_no_putch()
{
    asm("mov ah, 0xff");
    asm("int 0xb0");
    asm("mov ah, 0");
}

void readln(int length, char *buffer)
{
    asm("mov cx, [ss:bp+4]");
    asm("mov di, [ss:bp+6]");
    asm("int 0xb1");
}
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
#endif // __STDIO_H_INCLUDED
//-------------------------------------------------------------------------------