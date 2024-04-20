#include <stdlib.h>
#include <stdio.h>

int main()
{
    int* ptr = malloc(100);
    *ptr = 42;
    printf("tak\n");
    int** ptr2 = (int**)ptr;
    *ptr2 = "hello";
    printf("nie\n");
    return 0;
}

