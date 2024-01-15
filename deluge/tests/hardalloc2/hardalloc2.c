#include <stdfil.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    char* str = zhard_alloc(666666);
    printf("hard size = %zu\n", zhard_getallocsize(str));
    printf("soft size = %zu\n", zgetallocsize(str));
    strcpy(str + 555555, "hello, world!!!");
    printf("thingy = %s\n", str + 555555);
    zhard_free(str);
    return 0;
}

