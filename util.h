#pragma once
#define USCH unsigned char
#define NUM_ELEMENTS(array) ( sizeof(array) / sizeof((array)[0]) )

void hexdump(USCH* buf, size_t size);

