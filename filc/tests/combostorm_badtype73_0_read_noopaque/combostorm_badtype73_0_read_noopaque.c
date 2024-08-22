#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(40));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    *(int64_t*)(buf + 32) = 42;
    buf = (char*)(buf) + 0;
    char* f0 = *(char**)(buf + 0);
    char* f1 = *(char**)(buf + 16);
    int64_t f2 = *(int64_t*)(buf + 32);
    ZASSERT(!strcmp(f0, "hello"));
    ZASSERT(!strcmp(f1, "hello"));
    ZASSERT(f2 == 42);
    return 0;
}
