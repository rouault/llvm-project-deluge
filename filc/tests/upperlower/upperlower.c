#include <stdfil.h>

int main()
{
    char* ptr = zgc_alloc(0x6660);
    ptr += 42;
    ZASSERT((char*)zgetlower(ptr) + 42 == ptr);
    ZASSERT((char*)zgetupper(ptr) - 0x6660 == ptr - 42);
    return 0;
}

