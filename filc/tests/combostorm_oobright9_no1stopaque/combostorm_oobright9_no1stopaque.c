#include <stdfil.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
static char* hello = "hello";
static unsigned char value;
static void init_test(void)
{
    unsigned index;
    value = 42;
    bool good = false;
    while (!good) {
        good = true;
        for (index = sizeof(char*); index--;) {
            if (((char*)&hello)[index] == value) {
                good = false;
                break;
            }
        }
        if (good)
            break;
        value++;
    }
}
int main()
{
    init_test();
    char* buf = (malloc(26));
    *(int16_t*)(buf + 0) = value;
    *(int16_t*)(buf + 2) = value;
    *(int16_t*)(buf + 4) = value;
    *(int16_t*)(buf + 6) = value;
    *(int16_t*)(buf + 8) = value;
    *(int16_t*)(buf + 10) = value;
    *(int16_t*)(buf + 12) = value;
    *(int16_t*)(buf + 14) = value;
    *(int16_t*)(buf + 16) = value;
    *(int16_t*)(buf + 18) = value;
    *(int16_t*)(buf + 20) = value;
    *(int16_t*)(buf + 22) = value;
    *(int16_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 26208;
    int16_t f0 = *(int16_t*)(buf + 0);
    int16_t f1 = *(int16_t*)(buf + 2);
    int16_t f2 = *(int16_t*)(buf + 4);
    int16_t f3 = *(int16_t*)(buf + 6);
    int16_t f4 = *(int16_t*)(buf + 8);
    int16_t f5 = *(int16_t*)(buf + 10);
    int16_t f6 = *(int16_t*)(buf + 12);
    int16_t f7 = *(int16_t*)(buf + 14);
    int16_t f8 = *(int16_t*)(buf + 16);
    int16_t f9 = *(int16_t*)(buf + 18);
    int16_t f10 = *(int16_t*)(buf + 20);
    int16_t f11 = *(int16_t*)(buf + 22);
    int16_t f12 = *(int16_t*)(buf + 24);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    ZASSERT(f4 == value);
    ZASSERT(f5 == value);
    ZASSERT(f6 == value);
    ZASSERT(f7 == value);
    ZASSERT(f8 == value);
    ZASSERT(f9 == value);
    ZASSERT(f10 == value);
    ZASSERT(f11 == value);
    ZASSERT(f12 == value);
    return 0;
}
