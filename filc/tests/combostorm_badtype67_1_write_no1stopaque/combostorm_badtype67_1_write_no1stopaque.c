#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(int64_t*)(buf + 0) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    int64_t f1 = *(int64_t*)(buf + 16);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
