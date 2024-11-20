#include <stdlib.h>
#include "utils.h"

int main()
{
    char* ptr = malloc(16);
    *((char**)opaque(ptr + 16)) = "hello";
    return 0;
}
