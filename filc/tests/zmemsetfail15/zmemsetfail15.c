#include <stdfil.h>
#include "utils.h"

int main()
{
    char* array[4];
    unsigned i;
    for (i = 4; i--;)
        array[i] = "hello";
    zprintf("got this far\n");
    zmemset(array, 42, sizeof(array) - 1);
    for (i = 3; i--;) {
        ZASSERT(opaque(array[i]) == (char*)0x2a2a2a2a2a2a2a2alu);
        ZASSERT(!zhasvalidcap(array[i]));
    }
    ZASSERT(opaque(array[3]) == (char*)0x2a2a2a2a2a2a2alu);
    ZASSERT(!zhasvalidcap(array[3]));
    return 0;
}

