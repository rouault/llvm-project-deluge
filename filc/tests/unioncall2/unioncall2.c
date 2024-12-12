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

void useunion(MYUNION u)
{
    ZASSERT(strlen(u.y.str2) == 3);
    ZASSERT(u.y.z == 666);
}

void test2()
{
    MYUNION u;
    u.y.str2 = "foo";
    u.y.z = 666;
    useunion(u);
}

int main()
{
    test2();
    return 0;
}

