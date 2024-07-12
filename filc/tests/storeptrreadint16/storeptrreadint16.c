#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>

int main()
{
    char** ptr = (char**)opaque(malloc(0x6660));
    *ptr = "bro";
    opaque((void*)*(short*)ptr);
    return 0;
}
