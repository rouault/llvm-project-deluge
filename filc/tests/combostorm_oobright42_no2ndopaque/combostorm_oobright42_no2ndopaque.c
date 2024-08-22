#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 24) = 42;
    buf = (char*)(buf) + 26208;
    int64_t f0 = *(int64_t*)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
