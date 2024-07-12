#include <stdlib.h>
#include "utils.h"

int main()
{
    char* ptr = malloc(16);
    *((int*)opaque(ptr + 0x6660)) = 42;
    return 0;
}
