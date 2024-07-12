#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    char* ptr = (char*)opaque(malloc(0x6660));
    *ptr = 42;
    opaque(*(void**)opaque(ptr));
    return 0;
}
