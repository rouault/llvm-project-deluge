#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(40));
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)opaque(buf) + 26208;
    char* f0 = *(char**)(buf + 16);
    int64_t f1 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    return 0;
}
