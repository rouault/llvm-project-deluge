#include <stdfil.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    char** strstr = zhard_alloc(char*, 42);
    printf("hard size = %zu\n", zhard_getallocsize(strstr));
    printf("soft size = %zu\n", zgetallocsize(strstr));
    strstr[10] = zhard_alloc(char, 100);
    printf("hard size 2 = %zu\n", zhard_getallocsize(strstr[10]));
    printf("soft size 2 = %zu\n", zgetallocsize(strstr[10]));
    strcpy(strstr[10], "hello, world!!!");
    printf("thingy = %s\n", strstr[10]);
    zhard_free(strstr[10]);
    zhard_free(strstr);
    return 0;
}

