#include <string.h>
#include "utils.h"

int main()
{
    opaque((void*)memcmp((char*)opaque("hello") + 100, opaque("world"), 5));
    return 0;
}

