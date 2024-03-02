#include <stdfil.h>
#include <stdlib.h>
#include <string.h>

struct foo {
    char* s;
    int v[3];
    char* t;
};

struct bar {
    char* a;
    char* b;
};

int main()
{
    struct foo* f = zalloc(struct foo, 10);
    struct bar* b = zalloc(struct bar, 13);
    struct foo* f2 = zalloc(struct foo, 10);
    struct bar* b2 = zalloc(struct bar, 13);

    size_t index;
    size_t index2;
    
    for (index = 10; index--;) {
        f[index].s = zasprintf("s = %zu", index);
        for (index2 = 3; index2--;)
            f[index].v[index2] = index + index2;
        f[index].t = zasprintf("t = %zu", index);
    }

    for (index = 13; index--;) {
        b[index].a = zasprintf("a = %zu", index);
        b[index].b = zasprintf("b = %zu", index);
    }

    zmemmove_nullify(b2, f, sizeof(struct foo) * 10);
    zmemmove_nullify(f2, b, sizeof(struct foo) * 10);

    for (index = 13; index--;) {
        switch (index) {
        case 0:
            ZASSERT(b2[index].a);
            ZASSERT(!strcmp(b2[index].a, "s = 0"));
            break;
        case 2:
            ZASSERT(b2[index].a);
            ZASSERT(!strcmp(b2[index].a, "t = 1"));
            break;
        case 5:
            ZASSERT(b2[index].a);
            ZASSERT(!strcmp(b2[index].a, "s = 4"));
            break;
        case 7:
            ZASSERT(b2[index].a);
            ZASSERT(!strcmp(b2[index].a, "t = 5"));
            break;
        case 10:
            ZASSERT(b2[index].a);
            ZASSERT(!strcmp(b2[index].a, "s = 8"));
            break;
        case 12:
            ZASSERT(b2[index].a);
            ZASSERT(!strcmp(b2[index].a, "t = 9"));
            break;
        default:
            zprintf("index = %zu\n", index);
            ZASSERT(!b2[index].a);
            break;
        }
        switch (index) {
        case 2:
            ZASSERT(b2[index].b);
            ZASSERT(!strcmp(b2[index].b, "s = 2"));
            break;
        case 4:
            ZASSERT(b2[index].b);
            ZASSERT(!strcmp(b2[index].b, "t = 3"));
            break;
        case 7:
            ZASSERT(b2[index].b);
            ZASSERT(!strcmp(b2[index].b, "s = 6"));
            break;
        case 9:
            ZASSERT(b2[index].b);
            ZASSERT(!strcmp(b2[index].b, "t = 7"));
            break;
        default:
            zprintf("index = %zu\n", index);
            ZASSERT(!b2[index].b);
            break;
        }
    }

    zprintf("got this far\n");

    for (index = 10; index--;) {
        for (index2 = 3; index2--;)
            ZASSERT(!f2[index].v[index2]);
        switch (index) {
        case 0:
            ZASSERT(f2[index].s);
            ZASSERT(!strcmp(f2[index].s, "a = 0"));
            break;
        case 2:
            ZASSERT(f2[index].s);
            ZASSERT(!strcmp(f2[index].s, "b = 2"));
            break;
        case 4:
            ZASSERT(f2[index].s);
            ZASSERT(!strcmp(f2[index].s, "a = 5"));
            break;
        case 6:
            ZASSERT(f2[index].s);
            ZASSERT(!strcmp(f2[index].s, "b = 7"));
            break;
        case 8:
            ZASSERT(f2[index].s);
            ZASSERT(!strcmp(f2[index].s, "a = 10"));
            break;
        default:
            ZASSERT(!f2[index].s);
            break;
        }
        switch (index) {
        case 1:
            ZASSERT(f2[index].t);
            ZASSERT(!strcmp(f2[index].t, "a = 2"));
            break;
        case 3:
            ZASSERT(f2[index].t);
            ZASSERT(!strcmp(f2[index].t, "b = 4"));
            break;
        case 5:
            ZASSERT(f2[index].t);
            ZASSERT(!strcmp(f2[index].t, "a = 7"));
            break;
        case 7:
            ZASSERT(f2[index].t);
            ZASSERT(!strcmp(f2[index].t, "b = 9"));
            break;
        case 9:
            ZASSERT(f2[index].t);
            ZASSERT(!strcmp(f2[index].t, "a = 12"));
            break;
        default:
            ZASSERT(!f2[index].t);
            break;
        }
    }

    char* buf = zalloc(char, 100);
    const char* str = "tam to tak tego tam";
    zmemmove_nullify(buf, str, strlen(str) + 1);
    ZASSERT(!strcmp(buf, str));

    return 0;
}


