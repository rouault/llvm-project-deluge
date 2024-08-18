#include <stdfil.h>
#include <string.h>

int main()
{
    char* ptr = "hello";
    ZASSERT(!strcmp(ptr, "hello"));

    void* ptr2;
    
    ptr2 = zmkptr(ptr, 666);
    ZASSERT(ptr2 == (void*)666);
    ptr2 = zmkptr(ptr2, (unsigned long)ptr);
    ZASSERT(ptr2 == ptr);
    ZASSERT(!strcmp(ptr2, "hello"));

    ptr2 = zorptr(ptr, 3);
    ZASSERT((((unsigned long)ptr2) & 3) == 3);

    ptr2 = zandptr(ptr2, ~(unsigned long)3);
    ZASSERT(ptr2 == ptr);
    ZASSERT(!strcmp(ptr2, "hello"));

    ptr2 = zxorptr(ptr, 666);
    ZASSERT((unsigned long)ptr2 == (((unsigned long)ptr) ^ 666));

    ptr2 = zorptr("world", 7);
    ZASSERT((((unsigned long)ptr2) & 7) == 7);
    ptr2 = zretagptr(ptr, ptr2, ~(unsigned long)7);
    ZASSERT((((unsigned long)ptr2) & 7) == 7);
    ptr2 = zandptr(ptr2, ~(unsigned long)7);
    ZASSERT(ptr2 == ptr);
    ZASSERT(!strcmp(ptr2, "hello"));

    return 0;
}
