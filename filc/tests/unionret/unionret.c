#include <string.h>
#include <stdfil.h>

typedef union
{
    double x;
    const char* str;
} MYUNION;

MYUNION getunion()
{
    MYUNION u;
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

