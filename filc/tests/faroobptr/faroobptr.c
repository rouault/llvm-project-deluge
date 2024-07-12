#include <stdlib.h>
#include "utils.h"

int main()
{
    char* ptr = malloc(16);
    *((char**)opaque(ptr + 0x6660)) = "hello";
    return 0;
}
