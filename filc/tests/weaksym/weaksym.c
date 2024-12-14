#include <stdfil.h>

void __attribute__((weak)) foo();

int main()
{
    if (foo) {
        zprintf("seem to have foo\n");
        foo();
    } else
        zprintf("did not find foo\n");
    return 0;
}

