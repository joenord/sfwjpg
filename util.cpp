#include "util.h"

#include <stdio.h>

void hexdump(USCH* buf, size_t size)
{
    size_t I = 0;

    printf("Hexdump %d (0x%X) bytes.\n", size, size);

    for (I = 0; I < size; I++)
    {
        printf("%02X", buf[I]);

        if (I % 0x10 == 0xF)
        {
            printf(" (%X)\n", I + 1);
        }
        else
        {
            printf(" ");
        }
    }

    if (size % 0x10 != 0)
    {
        printf("\n");
    }
}
