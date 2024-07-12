#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    __int128* ptr = (__int128*)opaque(malloc(0x6660));
    *ptr = 666;
    opaque(*(void**)opaque(ptr));
    return 0;
}
