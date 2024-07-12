#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    short* ptr = (short*)opaque(NULL);
    opaque((void*)*ptr);
    return 0;
}
