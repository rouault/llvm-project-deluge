#include <stdio.h>

void* opaque(void* ptr);

static int foo(void)
{
    return 42;
}

int main()
{
    char* (*f)(void) = (char* (*)(void))opaque(foo);
    printf("f returned = %s\n", f());
    return 0;
}

