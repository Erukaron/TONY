#include "COM_HEAD.h"
#include "LZW.h"

void main(char argc, char *argv);

void main(char argc, char *argv)
{
    malloc(20);

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
