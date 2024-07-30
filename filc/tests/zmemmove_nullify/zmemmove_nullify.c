#include <stdfil.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

struct foo {
    char* s;
    int v[3];
    char* t;
};

struct bar {
    char* a;
    char* b;
};

static void foo_bar(struct foo* f, struct bar* b,
                    struct foo* f2, struct bar* b2)
{
    size_t index;
    size_t index2;
    
    for (index = 10; index--;) {
        f[index].s = zasprintf("s = %zu", index);
        for (index2 = 3; index2--;)
            f[index].v[index2] = index + index2;
        f[index].t = zasprintf("t = %zu", index);
    }

    for (index = 15; index--;) {
        b[index].a = zasprintf("a = %zu", index);
        b[index].b = zasprintf("b = %zu", index);
    }

    __builtin_memcpy(f2, f, sizeof(struct foo) * 10);
    __builtin_memcpy(b2, b, sizeof(struct bar) * 15);

    zmemmove_nullify(b2, f, sizeof(struct foo) * 10);
    zmemmove_nullify(f2, b, sizeof(struct foo) * 10);

    for (index = 15; index--;) {
        ZASSERT(!b2[index].a);
        ZASSERT(!b2[index].b);
    }

    for (index = 10; index--;) {
        for (index2 = 3; index2--;)
            ZASSERT(!f2[index].v[index2]);
        ZASSERT(!f2[index].s);
        ZASSERT(!f2[index].t);
    }

    zmemmove_nullify(f2, f, sizeof(struct foo) * 10);
    zmemmove_nullify(b2, b, sizeof(struct bar) * 15);

    for (index = 15; index--;) {
        ZASSERT(!b2[index].a);
        ZASSERT(!b2[index].b);
    }

    for (index = 10; index--;) {
        for (index2 = 3; index2--;)
            ZASSERT(f2[index].v[index2] == index + index2);
        ZASSERT(!f2[index].s);
        ZASSERT(!f2[index].t);
    }
}

int main()
{
    struct foo* f = zgc_alloc(sizeof(struct foo) * 10);
    struct bar* b = zgc_alloc(sizeof(struct bar) * 15);
    struct foo* f2 = zgc_alloc(sizeof(struct foo) * 10);
    struct bar* b2 = zgc_alloc(sizeof(struct bar) * 15);

    foo_bar(f, b, f2, b2);

    zprintf("doing it again\n");
    memset(f, 0, sizeof(struct foo) * 10);
    memset(b, 0, sizeof(struct bar) * 15);
    memset(f2, 0, sizeof(struct foo) * 10);
    memset(b2, 0, sizeof(struct bar) * 15);
    
    foo_bar(f2, b2, f, b);

    char* buf = zgc_alloc(100);
    const char* str = "tam to tak tego tam";
    zmemmove_nullify(buf, str, strlen(str) + 1);
    ZASSERT(!strcmp(buf, str));

    char buf2[16];
    unsigned i;
    for (i = 16; i--;)
        buf2[i] = 0;
    char* ptr;
    zmemmove_nullify(&ptr, buf2, 16);
    *(char**)opaque(&ptr) = "hello";
    ZASSERT(!strcmp(ptr, "hello"));

    return 0;
}


