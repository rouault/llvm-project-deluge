#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(48));
    *(__int128*)(buf + 0) = 42;
    *(__int128*)(buf + 16) = 42;
    *(char**)(buf + 32) = "hello";
    buf = (char*)(buf) + 0;
    __int128 f0 = *(__int128*)(buf + 0);
    __int128 f1 = *(__int128*)(buf + 16);
    __int128 f2 = *(__int128*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    return 0;
}
