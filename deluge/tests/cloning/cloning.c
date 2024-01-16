#include <stdfil.h>

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

int main(int argc, char** argv)
{
    struct foo* f = zalloc_zero(struct foo, 42);
    struct bar* b = zalloc_flex_zero(struct bar, d, 666);
    unsigned* a = zalloc_zero(unsigned, 100);

    ztype* ft = zgettype(f);
    struct foo* f2 = zalloc_with_type(ft, 1);
    __builtin_memcpy(f2, f + 10, sizeof(struct foo));

    ztype* ft2 = zgettypeslice(f + 5, sizeof(struct foo));
    ZASSERT(ft2 == ft);

    ztype* ft3 = zgettypeslice((char*)f + 8, sizeof(struct foo) - 8);
    void* thingy = zalloc_with_type(ft3, 1);
    __builtin_memcpy(thingy, (char*)(f + 30) + 8, sizeof(struct foo) - 8);

    zprintf("Spoko\n");
    return 0;
}

