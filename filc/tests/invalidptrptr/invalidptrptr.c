#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    char** ptr = (char**)opaque((void*)0x666666660);
    *ptr = "hello";
    return 0;
}
