#include <stdfil.h>

int foo(int x)
{
    return x + 42;
}

void* opaque(void*);

int main(void)
{
    char* x = (char*)opaque(foo);
    *x = 32;
    zprint("blah");
    return 0;
}

