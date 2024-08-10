#include <stdfil.h>
#include "utils.h"

static void foo(void)
{
    void (*recurse)(void) = opaque(foo);
    recurse();
}

int main()
{
    foo();
    ZASSERT(!"Should not get here");
    return 0;
}
