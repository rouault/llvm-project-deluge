#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(40));
    *(char**)(buf + 0) = "hello";
    *(int64_t*)(buf + 16) = 42;
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + -26208;
    char* f0 = *(char**)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    return 0;
}
