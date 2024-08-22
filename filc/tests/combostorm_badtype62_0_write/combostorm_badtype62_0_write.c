#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(16));
    *(char**)(buf + 0) = "hello";
    buf = (char*)opaque(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
    return 0;
}
