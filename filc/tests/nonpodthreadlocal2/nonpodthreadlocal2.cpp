#include <stdfil.h>

struct S { S(): x(666) {} int x; };
struct T { T(): x(42) {} int x; };
struct U { U(): x(1410) {} int x; };
void test5()
{
    thread_local S s;
    thread_local T t;
    thread_local U u;
    ZASSERT(s.x == 666);
    ZASSERT(t.x == 42);
    ZASSERT(u.x == 1410);
}

int main()
{
    test5();
    return 0;
}
