#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

int main()
{
    char* ptr = malloc(16);
    printf("value = %s\n", *((char**)opaque(ptr + 16)));
    return 0;
}
