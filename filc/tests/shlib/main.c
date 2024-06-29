#include <stdfil.h>

int thingy(void);
int stuff(void);
int foo(void);
int bar(void);
int blah(void);
int bleh(void);

int main()
{
    ZASSERT(thingy() == 666);
    ZASSERT(stuff() == 666);
    ZASSERT(foo() == 42);
    ZASSERT(bar() == 42);
    ZASSERT(blah() == 1410);
    ZASSERT(bleh() == 1410);
    return 0;
}

