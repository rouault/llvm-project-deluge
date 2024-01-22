#include <stdfil.h>
#include "utils.h"

struct foo {
    int x;
    double y;
    char* z;
    struct bar* w;
};

struct bar {
    char* a;
    struct foo* b;
    int c;
    float d[];
};

struct baz {
    char* a;
    struct foo* b;
    int c;
    float d;
};

struct foo_baz {
    struct foo foo;
    struct baz baz;
};

int main(int argc, char** argv)
{
    struct foo* f = zalloc_zero(struct foo, 42);
    struct bar* b = zalloc_flex_zero(struct bar, d, 666);
    unsigned* a = zalloc_zero(unsigned, 100);

    ztype* ft = zgettype(f);
    struct foo* f2 = zalloc_with_type(ft, sizeof(struct foo));
    __builtin_memcpy(f2, f + 10, sizeof(struct foo));

    ztype* ft2 = zgettypeslice(f + 5, sizeof(struct foo));
    ZASSERT(ft2 == ft);

    ztype* ft3 = zgettypeslice((char*)f + 8, sizeof(struct foo) - 8);
    void* thingy = zalloc_with_type(ft3, sizeof(struct foo) - 8);
    __builtin_memcpy(thingy, (char*)(f + 30) + 8, sizeof(struct foo) - 8);

    struct foo_baz* foo_baz = opaque(
        zalloc_with_type(zcattype(ztypeof(struct foo), sizeof(struct foo),
                                  ztypeof(struct baz), sizeof(struct baz)),
                         sizeof(struct foo) + sizeof(struct baz)));
    foo_baz->foo.x = 1;
    foo_baz->foo.y = 1.2;
    foo_baz->foo.z = "yo";
    foo_baz->foo.w = b;
    foo_baz->baz.a = "hej";
    foo_baz->baz.b = &foo_baz->foo;
    foo_baz->baz.c = 3;
    foo_baz->baz.d = 3.1;
    ZASSERT(zgettype(foo_baz) == zcattype(ztypeof(struct foo), sizeof(struct foo),
                                          ztypeof(struct baz), sizeof(struct baz)));

    zprintf("Spoko\n");
    return 0;
}

