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
    char* buf = (malloc(24));
    *(int8_t*)(buf + 7) = value;
    *(int8_t*)(buf + 15) = value;
    *(char**)(buf + 16) = hello;
    buf = (char*)(buf) + 0;
    int8_t f0 = *(int8_t*)(buf + 7);
    int8_t f1 = *(int8_t*)(buf + 15);
    int8_t f2 = *(int8_t*)(buf + 23);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    return 0;
}
