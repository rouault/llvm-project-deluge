#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 16);
    ZASSERT(f0 == 42);
    return 0;
}
