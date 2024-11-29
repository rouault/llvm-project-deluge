#include <stdfil.h>
#include <malloc.h>

int main()
{
    char* p = (char*)malloc(16);
    ZASSERT(malloc_usable_size(p) == 16);
    ZASSERT(malloc_usable_size(p + 8) == 8);
    ZASSERT(!malloc_usable_size(p + 42));
    ZASSERT(!malloc_usable_size(p - 42));
    ZASSERT(!malloc_usable_size((void*)666));
    return 0;
}

