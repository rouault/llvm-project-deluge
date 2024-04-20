#include <stdio.h>

void* opaque(void* ptr);

static const char* foo(void)
{
    return "hello";
}

int main()
{
    int (*f)(void) = (int (*)(void))opaque(foo);
    printf("f returned = %d\n", f());
    return 0;
}

