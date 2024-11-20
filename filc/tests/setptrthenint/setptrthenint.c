#include <stdlib.h>
#include <stdio.h>
#include <stdfil.h>
#include <inttypes.h>

int main()
{
    char** ptr = malloc(100);
    char* witam = "witam";
    *ptr = witam;
    printf("tak\n");
    int* ptr2 = (int*)ptr;
    *ptr2 = 666;
    printf("nie\n");
    ZASSERT(zgetlower(*ptr) == zgetlower(witam));
    ZASSERT(*ptr == (char*)((uintptr_t)666 | ((uintptr_t)witam & 0xffffffff00000000lu)));
    return 0;
}

