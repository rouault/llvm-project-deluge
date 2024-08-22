#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)opaque(buf) + 0;
    int32_t f0 = *(int32_t*)(buf + 16);
    ZASSERT(f0 == 42);
    return 0;
}
