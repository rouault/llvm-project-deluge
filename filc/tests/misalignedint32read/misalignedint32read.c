#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    int* ptr = (int*)((char*)opaque(malloc(0x6660)) + sizeof(int) / 2);
    opaque((void*)*ptr);
    return 0;
}
