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
    char* buf = (malloc(25));
    *(int8_t*)(buf + 0) = value;
    *(int8_t*)(buf + 2) = value;
    *(int8_t*)(buf + 4) = value;
    *(int8_t*)(buf + 6) = value;
    *(char**)(buf + 8) = hello;
    *(int8_t*)(buf + 16) = value;
    *(int8_t*)(buf + 18) = value;
    *(int8_t*)(buf + 20) = value;
    *(int8_t*)(buf + 22) = value;
    *(int8_t*)(buf + 24) = value;
    buf = (char*)opaque(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 0);
    int8_t f1 = *(int8_t*)(buf + 2);
    int8_t f2 = *(int8_t*)(buf + 4);
    int8_t f3 = *(int8_t*)(buf + 6);
    int8_t f4 = *(int8_t*)(buf + 8);
    int8_t f5 = *(int8_t*)(buf + 10);
    int8_t f6 = *(int8_t*)(buf + 12);
    int8_t f7 = *(int8_t*)(buf + 14);
    int8_t f8 = *(int8_t*)(buf + 16);
    int8_t f9 = *(int8_t*)(buf + 18);
    int8_t f10 = *(int8_t*)(buf + 20);
    int8_t f11 = *(int8_t*)(buf + 22);
    int8_t f12 = *(int8_t*)(buf + 24);
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
