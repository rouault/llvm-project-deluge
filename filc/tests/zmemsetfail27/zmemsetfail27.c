#include <stdfil.h>
#include <inttypes.h>
#include "utils.h"

int main()
{
    char* array[4];
    unsigned i;
    char* bruh = "bruh";
    for (i = 4; i--;)
        array[i] = bruh;
    zmemset((char*)array + 1, 0, 30);
    ZASSERT(opaque(array[0]) == (char*)((uintptr_t)bruh & 0xff));
    for (i = 1; i < 4; ++i)
        ZASSERT(!opaque(array[i]));
    for (i = 4; i--;)
        ZASSERT(!zhasvalidcap(array[i]));
    return 0;
}

