#include <stdfil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

void foo(void)
{
    zprintf("foo\n");
}

void bar(int a, int b, int c, int d, int e, int f, int g)
{
    ZASSERT(a + 3 == b);
    ZASSERT(b + 3 == c);
    ZASSERT(c + 3 == d);
    ZASSERT(d + 3 == e);
    ZASSERT(e + 3 == f);
    ZASSERT(f + 3 == g);
}

struct baz_arg_packet {
    char* a;
    int b;
};
void baz(char* a, int b, char* c, int d)
{
    ZASSERT(b + 1410 == d);
    ZASSERT(!strcmp(zasprintf("%d", b), a));
    ZASSERT(!strcmp(zasprintf("%d", d), c));
}

int thingy(char* x)
{
    int result;
    ZASSERT(sscanf(x, "%d", &result) == 1);
    return result;
}

char* stuff(int x)
{
    return zasprintf("%d", x);
}

int main()
{
    zprintf("calling foo1:");
    zcall_void(foo, NULL);
    zprintf("calling foo2:");
    ZASSERT(!zcall_int(foo, NULL));
    zprintf("calling foo3:");
    ZASSERT(!zcall_ptr(foo, NULL));

    int64_t* bar_args = malloc(100 * sizeof(int64_t));
    int index;
    for (index = 100; index--;)
        bar_args[index] = index * 3;
    zcall_void(bar, bar_args);
    zcall_void(bar, bar_args + 1);
    zcall_void(bar, bar_args + 7);
    ZASSERT(!zcall_int(bar, bar_args + 8));
    ZASSERT(!zcall_ptr(bar, bar_args + 11));

    struct baz_arg_packet* baz_args = malloc(100 * sizeof(struct baz_arg_packet));
    for (index = 100; index--;) {
        baz_args[index].a = zasprintf("%d", index * 1410);
        baz_args[index].b = index * 1410;
    }
    zcall_void(baz, baz_args);
    zcall_void(baz, baz_args + 1);
    zcall_void(baz, baz_args + 7);
    ZASSERT(!zcall_int(baz, baz_args + 8));
    ZASSERT(!zcall_ptr(baz, baz_args + 11));

    char* thingy_arg = "666";
    zcall_void(thingy, &thingy_arg);
    ZASSERT(zcall_int(thingy, &thingy_arg) == 666);

    int stuff_arg = 666;
    zcall_void(stuff, &stuff_arg);
    ZASSERT(!strcmp(zcall_ptr(stuff, &stuff_arg), "666"));
    
    return 0;
}
