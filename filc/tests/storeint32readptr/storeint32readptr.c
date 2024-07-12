#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    int* ptr = (int*)opaque(malloc(0x6660));
    *ptr = 666;
    opaque(*(void**)opaque(ptr));
    return 0;
}
