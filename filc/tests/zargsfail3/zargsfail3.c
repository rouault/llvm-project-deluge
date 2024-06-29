#include <stdio.h>
#include <stdfil.h>

static void thingy(char* x)
{
    int* ptr = zargs();
    printf("%d\n", *ptr);
}

int main()
{
    thingy("hello");
    return 0;
}

