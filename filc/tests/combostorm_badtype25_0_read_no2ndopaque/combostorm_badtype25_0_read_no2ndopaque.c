#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    return 0;
}
