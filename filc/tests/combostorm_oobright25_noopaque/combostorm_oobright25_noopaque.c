#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(24));
    *(int32_t*)(buf + 0) = 42;
    *(int32_t*)(buf + 20) = 42;
    buf = (char*)(buf) + 26208;
    int32_t f0 = *(int32_t*)(buf + 0);
    int32_t f1 = *(int32_t*)(buf + 20);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
