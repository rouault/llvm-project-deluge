#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    __int128* ptr = (__int128*)((char*)opaque(malloc(0x6660)) + sizeof(__int128) / 2);
    *ptr = 666;
    return 0;
}
