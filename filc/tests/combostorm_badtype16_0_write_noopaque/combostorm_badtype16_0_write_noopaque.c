#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
int main()
{
    char* buf = (malloc(20));
    *(char**)(buf + 0) = "hello";
    *(int16_t*)(buf + 18) = 42;
    buf = (char*)(buf) + 0;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 18);
    ZASSERT(f0 == 42);
    ZASSERT(f1 == 42);
    return 0;
}
