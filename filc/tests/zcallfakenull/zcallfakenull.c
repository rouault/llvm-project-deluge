#include <stdfil.h>
#include <stdlib.h>

static void foo(void) { }

int main()
{
    zcall(foo, zmkptr(malloc(16), 0));
    return 0;
}

