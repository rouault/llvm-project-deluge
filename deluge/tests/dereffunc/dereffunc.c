#include <stdfil.h>

int foo(int x)
{
    return x + 42;
}

void* opaque(void*);

int main(void)
{
    int* x = (int*)opaque(foo);
    *x = 666;
    zprint("blah");
    return 0;
}

