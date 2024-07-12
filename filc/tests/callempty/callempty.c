#include <stdfil.h>
#include "utils.h"

int main()
{
    void* empty = zgc_alloc(0);
    ZASSERT(zgetlower(empty) == zgetupper(empty));
    ZASSERT(empty == zgetlower(empty));
    zprintf("got here\n");
    ((int (*)(void))opaque(empty))();
    zprintf("wtf\n");
    return 0;
}

