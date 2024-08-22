#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    char* f1 = *(char**)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    return 0;
}
