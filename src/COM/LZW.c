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
#include "COM_HEAD.h"
#include "stdio.h"
#include "LZW.h"

void main(char argc, char *argv);

void main(char argc, char *argv)
{
    dbg
    reserved_memory new_mem;
    malloc(20, &new_mem);

    short s = new_mem.segment;
    byte o = new_mem.offset;
    short si = new_mem.size;

    char inp[20];
    printf("Hallo!");
    putch(':');
    readln(20, inp);

    putch(RETURN);
    putch(LINEFEED);
    printf(inp);

    char c = getch();
    putch(c);

    for (char i = 0; i < argc; i++)
    {
        putch(argv[i]);
    }

    printf(argv);
}
