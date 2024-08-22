#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(30));
    *(int8_t*)(buf + 9) = 42;
    *(int8_t*)(buf + 19) = 42;
    *(int8_t*)(buf + 29) = 42;
    buf = (char*)(buf) + 26208;
    int8_t f0 = *(int8_t*)(buf + 9);
    int8_t f1 = *(int8_t*)(buf + 19);
    int8_t f2 = *(int8_t*)(buf + 29);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    ZASSERT(f2 == 42);
    return 0;
}
