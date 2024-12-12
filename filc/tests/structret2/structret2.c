#include <string.h>
#include <stdfil.h>

typedef struct
{
    int x;
    const char* str;
} foo;

foo getunion()
{
    foo u;
    u.str = "foo";
    return u;
}

void test2()
{
    ZASSERT(strlen(getunion().str) == 3);
}

int main()
{
    test2();
    return 0;
}

