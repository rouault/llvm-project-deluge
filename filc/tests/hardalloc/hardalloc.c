#include <stdfil.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    char* str = zhard_alloc(char, 666);
    printf("hard size = %zu\n", zhard_getallocsize(str));
    printf("soft size = %zu\n", zgetallocsize(str));
    strcpy(str, "hello, world!!!");
    printf("thingy = %s\n", str);
    zhard_free(str);
    return 0;
}

