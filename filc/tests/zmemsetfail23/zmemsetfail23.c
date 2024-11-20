#include <stdfil.h>
#include <string.h>
#include <inttypes.h>
#include "utils.h"

int main()
{
    char* array[4];
    unsigned i;
    char* bruh = "bruh";
    for (i = 4; i--;)
        array[i] = bruh;
    zmemset((char*)array + 1, 0, 14);
    ZASSERT(opaque(array[0]) == (char*)((uintptr_t)bruh & 0xff));
    ZASSERT(!zhasvalidcap(array[0]));
    ZASSERT(!opaque(array[1]));
    ZASSERT(!zhasvalidcap(array[1]));
    ZASSERT(!strcmp(array[2], bruh));
    ZASSERT(!strcmp(array[3], bruh));
    return 0;
}

