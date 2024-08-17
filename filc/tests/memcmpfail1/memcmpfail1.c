#include <string.h>
#include "utils.h"

int main()
{
    opaque((void*)memcmp(opaque("hello"), opaque(main), 1));
    return 0;
}

