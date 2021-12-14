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
// Standard library
//-------------------------------------------------------------------------------
#ifndef __STDLIB_H_INCLUDED
#define __STDLIB_H_INCLUDED
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Typedefs
//-------------------------------------------------------------------------------
typedef struct 
{
    short segment;
    short offset;
    unsigned short size;
} reserved_memory;
asm(
    "struc reserved_memory\n"
    "   .segment resw 1\n"
    "   .offset  resw 1\n"
    "   .size    resw 1\n"
    "endstruc\n"
    );
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Prototypes
//-------------------------------------------------------------------------------
// int 0xb8
void malloc(unsigned short size, reserved_memory *result);
// int 0xb9
void free(reserved_memory *allocated);
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
void malloc(unsigned short size, reserved_memory *allocated)
{
    asm(
        "mov ax, [ss:bp+4]\n"
        "int 0xb8\n"
        "mov cx, bx\n"
        "mov bx, [ss:bp+6] ; reserved_memory pointer\n"
        "mov [ss:bx + reserved_memory.segment], cx\n"
        "mov [ss:bx + reserved_memory.offset], dx\n"
        "mov [ss:bx + reserved_memory.size], ax\n"
        );
}

void free(reserved_memory *allocated)
{
    asm(
        "mov bx, [ss:bp+4]\n"
        "mov ax, [ss:bx + reserved_memory.size]\n"
        "mov dx, [ss:bx + reserved_memory.offset]\n"
        "mov bx, [ss:bx + reserved_memory.segment]\n"
        "int 0x9b\n"
        );
}
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
#endif // __STDLIB_H_INCLUDED
//-------------------------------------------------------------------------------