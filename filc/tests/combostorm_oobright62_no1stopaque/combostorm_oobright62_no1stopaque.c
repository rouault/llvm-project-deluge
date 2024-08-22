#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(8));
    *(int64_t*)(buf + 0) = 42;
    buf = (char*)opaque(buf) + 26208;
    int64_t f0 = *(int64_t*)(buf + 0);
    ZASSERT(f0 == 42);
    return 0;
}
