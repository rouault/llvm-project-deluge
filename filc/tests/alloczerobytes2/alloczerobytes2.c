#include <stdfil.h>

static void check(char* p)
{
    ZASSERT(p);
    ZASSERT(zgetlower(p) == p);
    ZASSERT(zgetupper(p) == p);
    ZASSERT(!zinbounds(p));
    zvalidate_ptr(p);
}

int main()
{
    char* p = zalloc(0);
    check(p);

    *p = 'c';

    zprintf("noooo\n");
    
    return 0;
}

