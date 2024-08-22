#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(32));
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int64_t f0 = *(int64_t*)(buf + 24);
    ZASSERT(f0 == 42);
    return 0;
}
