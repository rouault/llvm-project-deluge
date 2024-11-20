#include <stdfil.h>
#include "utils.h"

int main()
{
    char* array[4];
    unsigned i;
    for (i = 4; i--;)
        array[i] = "hello";
    zprintf("got this far\n");
    zmemset(array, 42, sizeof(array));
    for (i = 4; i--;) {
        ZASSERT(opaque(array[i]) == (char*)0x2a2a2a2a2a2a2a2alu);
        ZASSERT(!zhasvalidcap(array[i]));
    }
    return 0;
}

