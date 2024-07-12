#include <stdfil.h>
#include "utils.h"

static int foo(void)
{
    return 666;
}

int main()
{
    ((int (*)(void))((char*)opaque(foo) + 1))();
    zprintf("wtf\n");
    return 0;
}
