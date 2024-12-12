#include <string.h>
#include <stdfil.h>

typedef union
{
    double x;
    const char* str;
    struct {
        const char* str2;
        int z;
    } y;
} MYUNION;

MYUNION getunion()
{
    MYUNION u;
    u.y.str2 = "foo";
    u.y.z = 666;
    return u;
}

void test2()
{
    MYUNION u = getunion();
    ZASSERT(strlen(u.y.str2) == 3);
    ZASSERT(u.y.z == 666);
}

int main()
{
    test2();
    return 0;
}

