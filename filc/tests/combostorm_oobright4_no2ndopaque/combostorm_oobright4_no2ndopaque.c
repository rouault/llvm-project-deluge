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
    char* buf = opaque(malloc(24));
    *(int8_t*)(buf + 5) = value;
    *(int8_t*)(buf + 11) = value;
    *(int8_t*)(buf + 17) = value;
    *(int8_t*)(buf + 23) = value;
    buf = (char*)(buf) + 26208;
    int8_t f0 = *(int8_t*)(buf + 5);
    int8_t f1 = *(int8_t*)(buf + 11);
    int8_t f2 = *(int8_t*)(buf + 17);
    int8_t f3 = *(int8_t*)(buf + 23);
    ZASSERT(f0 == value);
    ZASSERT(f1 == value);
    ZASSERT(f2 == value);
    ZASSERT(f3 == value);
    return 0;
}
