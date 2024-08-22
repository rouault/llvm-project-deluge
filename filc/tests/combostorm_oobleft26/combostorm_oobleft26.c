#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = opaque(malloc(20));
    *(int32_t*)(buf + 16) = 42;
    buf = (char*)opaque(buf) + -26208;
    ZASSERT(*(int32_t*)(buf + 16) == 42);
    return 0;
}
