#include <stdfil.h>

int thingy(void);
int stuff(void);
int foo(void);
int bar(void);
int blah(void);
int bleh(void);
int fuzz(void);
int buzz(void);
int fizz(void);
int wombat(int x, int y);
int baz(int x, int y);
int red(int x, int y);
int blue(int x, int y);
extern int green;
extern int yellow;

int main()
{
    ZASSERT(thingy() == 666);
    ZASSERT(stuff() == 666);
    ZASSERT(foo() == 42);
    ZASSERT(bar() == 42);
    ZASSERT(blah() == 1410);
    ZASSERT(bleh() == 1410);
    ZASSERT(fuzz() == 111);
    ZASSERT(buzz() == 111);
    ZASSERT(fizz() == 111);
    ZASSERT(wombat(4, 5) == 20);
    ZASSERT(baz(6, 7) == 42);
    ZASSERT(red(8, 9) == 17);
    ZASSERT(blue(9, 10) == 19);
    ZASSERT(green == 1000);
    ZASSERT(yellow == 1000);
    green = 2000;
    zcompiler_fence();
    ZASSERT(green == 2000);
    ZASSERT(yellow == 2000);
    return 0;
}

