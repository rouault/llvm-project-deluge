#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    long* ptr = (long*)((char*)opaque(malloc(0x6660)) + sizeof(long) / 2);
    *ptr = 666;
    return 0;
}
