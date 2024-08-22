#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(18));
    *(int8_t*)(buf + 0) = 42;
    *(int8_t*)(buf + 17) = 42;
    buf = (char*)opaque(buf) + -26208;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 17);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
