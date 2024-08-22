#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(18));
    *(int16_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + 26208;
    int16_t f0 = *(int16_t*)(buf + 16);
    ZASSERT(f0 == 42);
    return 0;
}
