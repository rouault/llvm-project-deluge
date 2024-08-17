#include <stdlib.h>
#include <string.h>
#include <stdfil.h>

struct foo {
    int x;
    int y;
    char* str;
    int z;
    int w;
};

struct bar {
    int a[4];
    char* str1;
    int b[4];
    char* str2;
    int c[4];
    char* str3;
    int d[4];
};

int main()
{
    struct foo a;

    a.x = 1;
    a.y = 2;
    a.str = "hello";
    a.z = 3;
    a.w = 4;

    struct foo b;
    memcpy(&b, &a, sizeof(a));
    ZASSERT(b.x == 1);
    ZASSERT(b.y == 2);
    ZASSERT(!strcmp(b.str, "hello"));
    ZASSERT(b.z == 3);
    ZASSERT(b.w == 4);

    struct foo c;
    memset(&c, 0, sizeof(c));
    memcpy(&c, &a, __builtin_offsetof(struct foo, w));
    ZASSERT(c.x == 1);
    ZASSERT(c.y == 2);
    ZASSERT(!strcmp(c.str, "hello"));
    ZASSERT(c.z == 3);
    ZASSERT(!c.w);

    struct foo d;
    memset(&d, 0, sizeof(d));
    memcpy(&d.y, &a.y, sizeof(struct foo) - __builtin_offsetof(struct foo, y));
    ZASSERT(!d.x);
    ZASSERT(d.y == 2);
    ZASSERT(!strcmp(d.str, "hello"));
    ZASSERT(d.z == 3);
    ZASSERT(d.w == 4);

    char** strings = malloc(sizeof(char*) * 42);
    size_t index;
    for (index = 42; index--;)
        strings[index] = zasprintf("index = %zu", index);

    memmove(strings, strings + 5, sizeof(char*) * 37);
    for (index = 37; index--;)
        ZASSERT(!strcmp(strings[index], zasprintf("index = %zu", index + 5)));
    for (index = 42; index-->37;)
        ZASSERT(!strcmp(strings[index], zasprintf("index = %zu", index)));
    
    for (index = 42; index--;)
        strings[index] = zasprintf("index = %zu", index);

    memmove(strings + 5, strings, sizeof(char*) * 37);
    for (index = 5; index--;)
        ZASSERT(!strcmp(strings[index], zasprintf("index = %zu", index)));
    for (index = 42; index-->37;)
        ZASSERT(!strcmp(strings[index], zasprintf("index = %zu", index - 5)));

    struct bar x;
    for (index = 4; index--;)
        x.a[index] = index + 42;
    x.str1 = "hello";
    for (index = 4; index--;)
        x.b[index] = index + 666;
    x.str2 = "world";
    for (index = 4; index--;)
        x.c[index] = index + 100;
    x.str3 = "!";
    for (index = 4; index--;)
        x.d[index] = (index + 42) * 666;
    memmove(x.a + 2, x.b + 2, (char*)(x.d + 2) - (char*)(x.b + 2));
    for (index = 2; index--;)
        ZASSERT(x.a[index] == index + 42);
    for (index = 4; index-->2;)
        ZASSERT(x.a[index] == index + 666);
    ZASSERT(!strcmp(x.str1, "world"));
    for (index = 4; index--;)
        ZASSERT(x.b[index] == index + 100);
    ZASSERT(!strcmp(x.str2, "!"));
    for (index = 2; index--;)
        ZASSERT(x.c[index] == (index + 42) * 666);
    for (index = 4; index-->2;)
        ZASSERT(x.c[index] == index + 100);
    ZASSERT(!strcmp(x.str3, "!"));
    for (index = 4; index--;)
        ZASSERT(x.d[index] == (index + 42) * 666);

    for (index = 4; index--;)
        x.a[index] = index + 42;
    x.str1 = "hello";
    for (index = 4; index--;)
        x.b[index] = index + 666;
    x.str2 = "world";
    for (index = 4; index--;)
        x.c[index] = index + 100;
    x.str3 = "!";
    for (index = 4; index--;)
        x.d[index] = (index + 42) * 666;
    memmove(x.b + 2, x.a + 2, (char*)(x.d + 2) - (char*)(x.b + 2));
    for (index = 4; index--;)
        ZASSERT(x.a[index] == index + 42);
    ZASSERT(!strcmp(x.str1, "hello"));
    for (index = 2; index--;)
        ZASSERT(x.b[index] == index + 666);
    for (index = 4; index-->2;)
        ZASSERT(x.b[index] == index + 42);
    ZASSERT(!strcmp(x.str2, "hello"));
    for (index = 4; index--;)
        ZASSERT(x.c[index] == index + 666);
    ZASSERT(!strcmp(x.str3, "world"));
    for (index = 2; index--;)
        ZASSERT(x.d[index] == index + 100);
    for (index = 4; index-->2;)
        ZASSERT(x.d[index] == (index + 42) * 666);

    return 0;
}

