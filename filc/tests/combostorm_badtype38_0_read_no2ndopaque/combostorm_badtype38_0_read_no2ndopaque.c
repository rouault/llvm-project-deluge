#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(40));
    *(int64_t*)(buf + 24) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    return 0;
}
