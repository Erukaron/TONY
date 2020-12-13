nasm -f bin BOOT_RTC.ASM -o TONY.IMG
tools\FAT_RootDir_Creator.exe -i "TONY.IMG" -fill 1474560
mkdir output
copy TONY.IMG output\TONY.IMG
make dbg