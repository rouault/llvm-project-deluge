#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    __int128* ptr = (__int128*)opaque((void*)0x666666660);
    opaque((void*)*ptr);
    return 0;
}
