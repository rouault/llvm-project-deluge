#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char* buf = malloc(100);
    free(buf);
    strcpy(buf, "hello");
    printf("%s\n", buf);
    return 0;
}

