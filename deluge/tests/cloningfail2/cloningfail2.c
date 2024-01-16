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

    zprintf("Spoko\n");
    
    __builtin_memcpy(f2, f - 1, sizeof(struct foo));

    zprintf("Kurza melodia\n");
    return 0;
}

