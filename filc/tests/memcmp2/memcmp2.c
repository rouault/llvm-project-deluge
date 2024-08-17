#include <stdfil.h>
#include <string.h>
#include "utils.h"

int main()
{
    ZASSERT(!memcmp(opaque("hello"), opaque("hello"), 5));
    ZASSERT(memcmp(opaque("world"), opaque("hello"), 5) > 0);
    ZASSERT(memcmp(opaque("hello"), opaque("world"), 5) < 0);

    char* ptr1 = "hello";
    char* ptr2 = ptr1;
    char* ptr3 = "world";
    ZASSERT(!memcmp(opaque(&ptr1), opaque(&ptr2), sizeof(ptr1)));
    ZASSERT(memcmp(opaque(&ptr1), opaque(&ptr3), sizeof(ptr1)));
    ZASSERT(memcmp(opaque(&ptr1), opaque("hello"), 5));
    ZASSERT(memcmp(opaque("hello"), opaque(&ptr1), 5));
    
    return 0;
}

