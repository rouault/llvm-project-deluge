#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(24));
    *(int64_t*)(buf + 0) = 42;
    *(int64_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 26208;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
