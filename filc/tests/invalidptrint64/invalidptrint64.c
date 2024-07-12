#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    long* ptr = (long*)opaque((void*)0x666666660);
    *ptr = 666;
    return 0;
}
