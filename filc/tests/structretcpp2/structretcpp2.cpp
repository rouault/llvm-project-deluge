#include <string.h>
#include <stdfil.h>

typedef struct
{
    const char* str;
    int x;
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

int main(int argc, char** argv)
{
    test2();
    return 0;
}

