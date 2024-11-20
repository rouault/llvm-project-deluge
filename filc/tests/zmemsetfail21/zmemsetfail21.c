#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include "utils.h"

int main()
{
    char* array[4];
    unsigned i;
    char* bruh = "bruh";
    for (i = 4; i--;)
        array[i] = bruh;
    zmemset((char*)array + 15, 0, 1);
    zprintf("array = %O\n", array);
    zprintf("array[1] = %P\n", array[1]);
    ZASSERT(opaque(array[0]) == bruh);
    ZASSERT(zhasvalidcap(array[0]));
    ZASSERT(opaque(array[1]) == bruh);
    ZASSERT(!zhasvalidcap(array[1]));
    ZASSERT(opaque(array[2]) == bruh);
    ZASSERT(zhasvalidcap(array[2]));
    ZASSERT(opaque(array[3]) == bruh);
    ZASSERT(zhasvalidcap(array[3]));
    return 0;
}

