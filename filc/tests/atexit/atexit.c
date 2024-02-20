#include <stdlib.h>
#include <stdio.h>

void thing(void)
{
    printf("Hello, world!\n");
}

int main(int argc, char** argv)
{
    atexit(thing);
    return 0;
}

