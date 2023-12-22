#include <stdio.h>
#include <stdlib.h>

int deluded_main(void);

void deluded_print(const char* string, void*, void*, void*)
{
    printf("%s", string);
}

void deluded_fail(void)
{
    printf("FAIL\n");
    exit(1);
}

int main(int argc, char** argv)
{
    return deluded_main();
}

