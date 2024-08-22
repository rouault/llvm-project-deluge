#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    ZASSERT(*(__int128*)(buf + 16) == 42);
    return 0;
}
