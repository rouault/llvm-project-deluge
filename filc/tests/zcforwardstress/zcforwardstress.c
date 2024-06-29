#include <stdfil.h>
#include <stdio.h>
#include <string.h>

static char* foo(char* x, char* y)
{
    char* result;
    asprintf(&result, "%s %s", x, y);
    return result;
}

static char* bar(char* x, char* y)
{
    zcforward(foo);
}

int main()
{
    unsigned i;
    for (i = 10000; i--;)
        ZASSERT(!strcmp(bar("hello", "world"), "hello world"));
    return 0;
}

