#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char* buf = malloc(100);
    strcpy(buf, "hello");
    free(buf);

    size_t i;
    for (i = 10000; i--;) {
        char* buf2 = malloc(100);
        snprintf(buf2, 100, "%zu", i);
        free(buf2);
    }
    
    printf("%s\n", buf);
    return 0;
}

