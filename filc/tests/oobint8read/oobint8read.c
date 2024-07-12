#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

int main()
{
    char* ptr = malloc(16);
    printf("value = %d\n", (int)((char*)opaque(ptr))[16]);
    return 0;
}
