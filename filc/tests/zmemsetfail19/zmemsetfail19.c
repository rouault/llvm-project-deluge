#include <stdfil.h>
#include <inttypes.h>

int main()
{
    char* array[4];
    unsigned i;
    char* bruh = "bruh";
    for (i = 4; i--;)
        array[i] = bruh;
    zmemset(array, 0, 1);
    ZASSERT(array[0] == (char*)((uintptr_t)bruh & ~(uintptr_t)0xff));
    ZASSERT(!zhasvalidcap(array[0]));
    for (i = 1; i < 4; ++i)
        ZASSERT(array[i] == bruh);
    return 0;
}

