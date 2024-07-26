#include <stdfil.h>
#include <string.h>

struct foo_args {
    int x;
    int y;
};
static void foo(int x, int y)
{
    struct foo_args* args = zargs();
    ZASSERT(args->x == x);
    ZASSERT(args->y == y);
    ZASSERT(x == y * 666);
}

struct bar_args {
    int x;
    int y;
    int z;
    int w;
    int a;
    int b;
    int c;
    int d;
};
static void bar(int x, int y, int z, int w, int a, int b, int c, int d)
{
    struct bar_args* args = zargs();
    ZASSERT(args->x == x);
    ZASSERT(args->y == y);
    ZASSERT(args->z == z);
    ZASSERT(args->w == w);
    ZASSERT(args->a == a);
    ZASSERT(args->b == b);
    ZASSERT(args->c == c);
    ZASSERT(args->d == d);
    ZASSERT(x * 2 == y);
    ZASSERT(x * 3 == z);
    ZASSERT(x * 4 == w);
    ZASSERT(x * 5 == a);
    ZASSERT(x * 6 == b);
    ZASSERT(x * 7 == c);
    ZASSERT(x * 8 == d);
}

struct baz_args {
    char* a;
    int b;
    char* c;
    int d;
};
static void baz(char* a, int b, char* c, int d)
{
    struct baz_args* args = zargs();
    ZASSERT(args->a == a);
    ZASSERT(args->b == b);
    ZASSERT(args->c == c);
    ZASSERT(args->d == d);
    ZASSERT(!strcmp(zasprintf("%d", b), a));
    ZASSERT(!strcmp(zasprintf("%d", d), c));
}

int main()
{
    foo(0, 0);
    foo(-666, -1);
    foo(666, 1);
    foo(6660, 10);
    bar(1, 2, 3, 4, 5, 6, 7, 8);
    bar(-1, -2, -3, -4, -5, -6, -7, -8);
    bar(2, 4, 6, 8, 10, 12, 14, 16);
    baz("42", 42, "666", 666);
    return 0;
}

