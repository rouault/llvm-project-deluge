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
    buf = (char*)opaque(buf) + 0;
    ZASSERT(!strcmp(*(char**)(buf + 0), "hello"));
    ZASSERT(*(__int128*)(buf + 32) == 42);
    return 0;
}
