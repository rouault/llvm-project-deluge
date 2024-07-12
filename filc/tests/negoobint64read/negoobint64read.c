#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

int main()
{
    char* ptr = malloc(16);
    printf("value = %d\n", (int)*((long*)opaque(ptr + -0x6660)));
    return 0;
}
