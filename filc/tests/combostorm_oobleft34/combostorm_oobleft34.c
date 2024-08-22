#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(__int128*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + -26208;
    ZASSERT(*(__int128*)(buf + 16) == 42);
    return 0;
}
