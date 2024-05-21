#include <stdfil.h>
#include <string.h>

int main()
{
    ZASSERT(!memcmp("hello", "hello", 5));
    ZASSERT(memcmp("world", "hello", 5) > 0);
    ZASSERT(memcmp("hello", "world", 5) < 0);

    char* ptr1 = "hello";
    char* ptr2 = ptr1;
    char* ptr3 = "world";
    ZASSERT(!memcmp(&ptr1, &ptr2, sizeof(ptr1)));
    ZASSERT(memcmp(&ptr1, &ptr3, sizeof(ptr1)));
    ZASSERT(memcmp(&ptr1, "hello", 5));
    ZASSERT(memcmp("hello", &ptr1, 5));
    
    return 0;
}

