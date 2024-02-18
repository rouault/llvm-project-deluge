#include <stdfil.h>
#include "utils.h"
#include <string.h>

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

struct thingy {
    char* a;
    int b;
    char* c;
    int d;
};

int main(int argc, char** argv)
{
    struct foo* f = zalloc(struct foo, 42);
    struct bar* b = zalloc_flex(struct bar, d, 666);
    unsigned* a = zalloc(unsigned, 100);

    ztype* ft = zgettype(f);
    struct foo* f2 = zalloc_with_type(ft, sizeof(struct foo));
    __builtin_memcpy(f2, f + 10, sizeof(struct foo));

    ztype* ft2 = zgettypeslice(f + 5, sizeof(struct foo));
    ZASSERT(ft2 == ft);

    ztype* ft3 = zgettypeslice((char*)f + 16, sizeof(struct foo) - 16);
    void* thingy = zalloc_with_type(ft3, sizeof(struct foo) - 16);
    __builtin_memcpy(thingy, (char*)(f + 30) + 16, sizeof(struct foo) - 16);

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

    struct baz* baz_whatever = opaque(
        zalloc_with_type(zcattype(ztypeof(struct baz), sizeof(struct baz),
                                  ztypeof(char), 13),
                         sizeof(struct baz) + 13));
    baz_whatever->a = "tak";
    baz_whatever->b = f;
    baz_whatever->c = 5;
    baz_whatever->d = 6.1;
    char* whatever = (char*)(baz_whatever + 1);
    strcpy(whatever, "hello, world");

    struct thingy* t = opaque(
        zalloc_with_type(zcattype(ztypeof(struct thingy), sizeof(struct thingy) * 10,
                                  ztypeof(double), sizeof(double) * 666),
                         sizeof(struct thingy) * 10 + sizeof(double) * 666));
    size_t i;
    for (i = 10; i--;) {
        t[i].a = "chrona";
        t[i].b = i;
        t[i].c = "zalew";
        t[i].d = 42;
    }
    double* d = (double*)(t + 10);
    for (i = 666; i--;)
        d[i] = i + 0.666;
    
    zprintf("Spoko\n");
    return 0;
}

