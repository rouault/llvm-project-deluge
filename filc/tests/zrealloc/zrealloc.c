#include <stdfil.h>
#include <string.h>
#include <stdio.h>

struct foo;
struct bar;
typedef struct foo foo;
typedef struct bar bar;

struct foo {
    int x;
    char* y;
};

struct bar {
    foo* f;
    double d;
};

static void setindex(bar* b, size_t index)
{
    zprintf("setindex at %zu\n", index);
    ZASSERT(!b[index].f);
    ZASSERT(!b[index].d);
    b[index].f = zgc_alloc(sizeof(foo));
    b[index].f->x = index;
    b[index].f->y = zasprintf("%zu", index);
    b[index].d = (double)index * 6.66;
}

static void checkindex(bar* b, size_t index)
{
    ZASSERT(b[index].f);
    ZASSERT((size_t)b[index].f->x == index);
    ZASSERT(!strcmp(b[index].f->y, zasprintf("%zu", index)));
    ZASSERT(b[index].d == (double)index * 6.66);
}

int main()
{
    char* buf = strdup("hello");
    char* buf2 = zgc_realloc(buf, 100);
    ZASSERT(!strcmp(buf2, "hello"));
    const char* str = "gjdhas;ofjdkagjdsai;gjdfpakgjdfkslajgdkls;ajhgdkfls;hgfkldzhgfjlk;dahglj;fdsag";
    strcpy(buf2, str);
    ZASSERT(!strcmp(buf2, str));
    char* buf3 = zgc_realloc(zgc_realloc(buf2, 3), 15);
    ZASSERT(!strncmp(buf3, "gjd", 3));
    strcpy(buf3, "Hello, world!\n");
    printf("%s", buf3);

    char* buf4 = zgc_realloc(zgc_realloc(buf3, 0), 6);
    sprintf(buf4, "Witaj");
    printf("%s\n", buf4);

    bar* b = zgc_alloc(sizeof(bar) * 5);
    size_t index;
    for (index = 5; index--;)
        setindex(b, index);
    for (index = 5; index--;)
        checkindex(b, index);

    zprintf("Got this far.\n");

    bar* b2 = zgc_realloc(b, sizeof(bar) * 10);
    for (index = 5; index--;)
        checkindex(b2, index);
    for (index = 10; index-->5;)
        setindex(b2, index);
    for (index = 10; index--;)
        checkindex(b2, index);

    zprintf("Past that part.\n");

    bar* b3 = zgc_realloc(zgc_realloc(b2, sizeof(bar) * 3), sizeof(bar) * 6);
    for (index = 3; index--;)
        checkindex(b3, index);
    for (index = 6; index-->3;)
        setindex(b3, index);
    for (index = 6; index--;)
        checkindex(b3, index);

    zprintf("Almost done.\n");

    bar* b4 = zgc_realloc(zgc_realloc(b3, 0), sizeof(bar) * 6);
    zprintf("Here.\n");
    for (index = 6; index--;)
        setindex(b4, index);
    for (index = 6; index--;)
        checkindex(b4, index);

    return 0;
}

