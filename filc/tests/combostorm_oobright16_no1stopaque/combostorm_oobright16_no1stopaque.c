#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(20));
    *(int16_t*)(buf + 0) = 42;
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)opaque(buf) + 26208;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
