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
    struct foo* f = zgc_alloc(sizeof(struct foo) * 42);
    unsigned index;
    for (index = 42; index--;) {
        f[index].z = 0;
        f[index].w = 0;
    }
    
    struct bar* b = zgc_alloc(__builtin_offsetof(struct bar, d) + 666 * sizeof(float));
    unsigned* a = zgc_alloc(sizeof(unsigned) * 100);

    struct foo* f2 = zgc_alloc(zlength(f));

    zprintf("Spoko\n");
    
    __builtin_memcpy(f2, (char*)(f + 10) + 1, sizeof(struct foo));

    zprintf("Kurza melodia\n");

    for (index = zlength(f2); index--;)
        ZASSERT(!((char*)f2)[index]);
    
    return 0;
}

