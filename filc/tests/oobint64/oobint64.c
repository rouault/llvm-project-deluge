#include <stdlib.h>
#include "utils.h"

int main()
{
    char* ptr = malloc(16);
    *((long*)opaque(ptr + 12)) = 42;
    return 0;
}
