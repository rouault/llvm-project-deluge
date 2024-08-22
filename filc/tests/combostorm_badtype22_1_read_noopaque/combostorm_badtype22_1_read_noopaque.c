#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(36));
    *(int32_t*)(buf + 8) = 42;
    *(int32_t*)(buf + 20) = 42;
    *(int32_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 8);
    char* f1 = *(char**)(buf + 16);
    int32_t f2 = *(int32_t*)(buf + 32);
    ZASSERT(f0 == 42);
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
    return 0;
}
