#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

int main()
{
    char* ptr = malloc(16);
    printf("value = %d\n", (int)*((long*)opaque(ptr + 12)));
    return 0;
}
