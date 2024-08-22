#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 0) = "hello";
    *(int32_t*)(buf + 28) = 42;
    buf = (char*)(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 12);
    int32_t f1 = *(int32_t*)(buf + 28);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
