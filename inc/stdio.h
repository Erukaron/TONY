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
typedef char* string;
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
// int 0x90
void putch(char c);
// int 0x91
void printf(string str);
// int 0x92
void printf_loc(char *str, char y, char x);
// int 0x9b
void flip();
void set_double_buffer(char enable);

// int 0xb0
char getch();
char getch_no_putch();
// int 0xb1
void readln(unsigned int length, char *buffer);
void readln_pw(unsigned int length, char *buffer, char obfuscator);

// int 0xb8
void malloc(unsigned short size, reserved_memory *result);
// int 0xb9
void dealloc(reserved_memory *allocated);
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

void printf(string str)
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

void flip()
{
    asm(
        "mov ah, 0\n"
        "int 0x9b\n"
        );
}

void set_double_buffer(char enable)
{
    if (enable)
    {
        asm(
            "mov ah, 1\n"
            "int 0x9b\n"
            );       
    }
    else
    {
        asm(
            "mov ah, 2\n"
            "int 0x9b\n"
            );       
    }
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

void readln(unsigned int length, char *buffer)
{
    readln_pw(length, buffer, 0xff);
}

void readln_pw(unsigned int length, char *buffer, char obfuscator)
{
    length--; // Remove 1 byte for terminator
    unsigned int maxInputLength = length;
    char userEntry = 0;

    while(userEntry != (char)RETURN)
    {
        userEntry = getch_no_putch();

        if (userEntry == (char)BACKSPACE)
        {
            // For Backspace there must be enough space in the buffer
            if (length < maxInputLength)
            {
                // Decrease buffer, set buffer to null and increase the remaining length
                buffer--;
                *buffer = 0;
                length++;

                putch((char)BACKSPACE);
                flip();
            }
        }
        else if (length > (unsigned int)0 && userEntry != (char)RETURN)
        {
            // Show entry
            if (obfuscator == (char)0xff)
                putch(userEntry);
            else 
                putch(obfuscator);

            flip();

            // Write entry, increase buffer and remaining length
            *buffer = userEntry;
            buffer++;
            length--;
        }
    }

    getch_no_putch(); // getch lf remaining in buffer
    putch((char)RETURN);
    putch((char)LINEFEED);
    flip();

    *buffer = 0; // null-terminate string

    /*
    asm("mov cx, [ss:bp+4]");
    asm("mov di, [ss:bp+6]");
    asm("int 0xb1");
    */
}

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

void dealloc(reserved_memory *allocated)
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
#endif // __STDIO_H_INCLUDED
//-------------------------------------------------------------------------------