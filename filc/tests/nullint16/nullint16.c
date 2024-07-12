#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    short* ptr = (short*)opaque(NULL);
    *ptr = 666;
    return 0;
}
