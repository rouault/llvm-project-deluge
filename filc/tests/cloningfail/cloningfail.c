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
    struct foo* f = zalloc(struct foo, 42);
    struct bar* b = zalloc_flex(struct bar, d, 666);
    unsigned* a = zalloc(unsigned, 100);

    ztype* ft = zgettype(f);
    struct foo* f2 = zalloc_with_type(ft, sizeof(struct foo));

    zprintf("Spoko\n");
    
    __builtin_memcpy(f2, (char*)(f + 10) + 1, sizeof(struct foo));

    zprintf("Kurza melodia\n");
    return 0;
}

