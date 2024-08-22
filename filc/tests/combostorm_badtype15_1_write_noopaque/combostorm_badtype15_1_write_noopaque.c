#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(32));
    *(int16_t*)(buf + 10) = 42;
    *(char**)(buf + 16) = "hello";
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 10);
    int16_t f1 = *(int16_t*)(buf + 22);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
