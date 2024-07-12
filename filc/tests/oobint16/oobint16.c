#include <stdlib.h>
#include "utils.h"

int main()
{
    char* ptr = malloc(16);
    *((short*)opaque(ptr + 15)) = 42;
    return 0;
}
