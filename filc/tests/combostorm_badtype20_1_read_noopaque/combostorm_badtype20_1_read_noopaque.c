#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(int32_t*)(buf + 4) = 42;
    *(int32_t*)(buf + 12) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 4);
    int32_t f1 = *(int32_t*)(buf + 12);
    char* f2 = *(char**)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(!strcmp(f2, "hello"));
    return 0;
}
