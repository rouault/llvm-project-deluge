#include <stdfil.h>
#include <string.h>

int main()
{
    char* array[4];
    zmemset(array, 42, sizeof(array) - 1);
    unsigned i;
    for (i = 3; i--;) {
        ZASSERT(array[i] == (char*)0x2a2a2a2a2a2a2a2alu);
        ZASSERT(!zhasvalidcap(array[i]));
    }
    ZASSERT(array[3] == (char*)0x2a2a2a2a2a2a2alu);
    ZASSERT(!zhasvalidcap(array[3]));
    zprintf("got this far\n");
    for (i = 4; i--;)
        array[i] = "hello";
    for (i = 4; i--;)
        ZASSERT(!strcmp(array[i], "hello"));
    return 0;
}

