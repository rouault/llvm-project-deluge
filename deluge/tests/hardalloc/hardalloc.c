#include <stdfil.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    char* str = zhard_alloc(666);
    strcpy(str, "hello, world!!!");
    printf("thingy = %s\n", str);
    zhard_free(str);
    return 0;
}

