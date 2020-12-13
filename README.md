# TONY
 T(iny) o(perati)n(g) (s)y(stem)
-------------------------------------------------

-------------------------------------------------
How to build:
    - Requirements:
        - nasm needs to be in the path variable
        - .Net framework 4.7.2 needs to be installed to run FAT_RootDir_Creator -> Creates the root directory, the fat and is used to alter file sizes
    - Run MAKE.BAT from the main directory (TONY.IMG is located in output folder after a successful build)
    - Test by: "MAKE.BAT test" or "MAKE.BAT dbg" -> a copy of Bochs-2.6.11 needs to be installed to tools directory
-------------------------------------------------

-------------------------------------------------
Floppy layout:
0x00000 - 0x001ff | 01 | 001 - 002 : Bootsector
0x00200 - 0x013ff | 09 | 002 - 011 : FAT
0x01400 - 0x025ff | 09 | 012 - 021 : FAT Backup
0x02600 - 0x041ff | 15 | 022 - 037 : Rootdir
0x04200 - 0x0???? | ?? | 038 - 0?? : Kernel
0x0???? -         |    | 0??       : EE
-------------------------------------------------

-------------------------------------------------
Memory layout (boot):
0x000000 - 0x0004ff : Interrupt vector table and bios data (1,25 KB)
0x007c00 - 0x007dff : Bootloader (0,5 KB)
0x007e00 - 0x009a00 : Root Dir (7 KB) / FAT (4,5 KB)
0x010000 - 0x01ffff : Stack (64 KB)

Memory layout:
0x000000 - 0x0004ff : Interrupt vector table and bios data (1,25 KB)
0x000500 - 0x0005ff : Reserved on some BIOSes (0,25 KB)
0x000600 - 0x0021ff : Root Dir (7 KB)
0x002200 - 0x0033ff : FAT (4,5 KB)
0x003400 - 0x00ffff : Kernel (51 KB)
0x010000 - 0x01ffff : Stack (64 KB)
0x020000 - 0x07ffff : free
                    : BIOS and video memory
0x100000            : free
-------------------------------------------------

-------------------------------------------------
MIT License

Copyright (c) 2020 Erukaron

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
-------------------------------------------------
