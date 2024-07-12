#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    short* ptr = (short*)((char*)opaque(malloc(0x6660)) + sizeof(short) / 2);
    opaque((void*)*ptr);
    return 0;
}
