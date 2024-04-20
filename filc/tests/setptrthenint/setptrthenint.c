#include <stdlib.h>
#include <stdio.h>

int main()
{
    char** ptr = malloc(100);
    *ptr = "witam";
    printf("tak\n");
    int* ptr2 = (int*)ptr;
    *ptr2 = 666;
    printf("nie\n");
    return 0;
}

