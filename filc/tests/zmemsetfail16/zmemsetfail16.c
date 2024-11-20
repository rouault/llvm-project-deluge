#include <stdfil.h>
#include <inttypes.h>

int main()
{
    char* array[4];
    unsigned i;
    char* hello = "hello";
    for (i = 4; i--;)
        array[i] = hello;
    zprintf("got this far\n");
    zmemset((char*)array + 1, 42, sizeof(array) - 1);
    ZASSERT(array[0] == (char*)((uintptr_t)0x2a2a2a2a2a2a2a00lu | ((uintptr_t)hello & 0xff)));
    ZASSERT(!zhasvalidcap(array[0]));
    for (i = 1; i < 4; ++i) {
        ZASSERT(array[i] == (char*)0x2a2a2a2a2a2a2a2alu);
        ZASSERT(!zhasvalidcap(array[i]));
    }
    return 0;
}

