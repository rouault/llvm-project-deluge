#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)(buf) + -26208;
    __int128 f0 = *(__int128*)(buf + 16);
    ZASSERT(f0 == 42);
    return 0;
}
