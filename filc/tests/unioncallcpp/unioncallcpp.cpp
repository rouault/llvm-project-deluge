#include <string.h>
#include <stdfil.h>

typedef union
{
    double x;
    const char* str;
} MYUNION;

void useunion(MYUNION u)
{
    ZASSERT(strlen(u.str) == 3);
}

void test2()
{
    MYUNION u;
    u.str = "foo";
    useunion(u);
}

int main(int argc, char** argv)
{
    test2();
    return 0;
}

