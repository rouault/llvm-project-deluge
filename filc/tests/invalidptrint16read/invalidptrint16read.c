#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    short* ptr = (short*)opaque((void*)0x666666660);
    opaque((void*)*ptr);
    return 0;
}
