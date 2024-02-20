#include <stdfil.h>

struct foo {
    int x;
    int y;
};

struct bar {
    double x;
    double y;
};

struct baz {
    char* x;
    char* y;
};

struct stuff {
    int x;
    int y;
    int z;
    int w;
};

struct thingy {
    double x;
    double y;
    double z;
    double w;
};

struct blah {
    char* x;
    char* y;
    char* z;
    char* w;
};

void printfoo(struct foo foo)
{
    zprintf("foo.x = %d\n", foo.x);
    zprintf("foo.y = %d\n", foo.y);
}

void printbar(struct bar bar)
{
    zprintf("bar.x = %lf\n", bar.x);
    zprintf("bar.y = %lf\n", bar.y);
}

void printbaz(struct baz baz)
{
    zprintf("baz.x = %s\n", baz.x);
    zprintf("baz.y = %s\n", baz.y);
}

void printstuff(struct stuff stuff)
{
    zprintf("stuff.x = %d\n", stuff.x);
    zprintf("stuff.y = %d\n", stuff.y);
    zprintf("stuff.z = %d\n", stuff.z);
    zprintf("stuff.w = %d\n", stuff.w);
}

void printthingy(struct thingy thingy)
{
    zprintf("thingy.x = %lf\n", thingy.x);
    zprintf("thingy.y = %lf\n", thingy.y);
    zprintf("thingy.z = %lf\n", thingy.z);
    zprintf("thingy.w = %lf\n", thingy.w);
}

void printblah(struct blah blah)
{
    zprintf("blah.x = %s\n", blah.x);
    zprintf("blah.y = %s\n", blah.y);
    zprintf("blah.z = %s\n", blah.z);
    zprintf("blah.w = %s\n", blah.w);
}

int main(int argc, char** argv)
{
    struct foo foo;
    foo.x = 1;
    foo.y = 2;
    printfoo(foo);
    struct bar bar;
    bar.x = 1.5;
    bar.y = 2.3;
    printbar(bar);
    struct baz baz;
    baz.x = "hello";
    baz.y = "world";
    printbaz(baz);
    struct stuff stuff;
    stuff.x = 3;
    stuff.y = 4;
    stuff.z = 5;
    stuff.w = 6;
    printstuff(stuff);
    struct thingy thingy;
    thingy.x = 3.8;
    thingy.y = 4.1;
    thingy.z = 5.3;
    thingy.w = 6.6;
    printthingy(thingy);
    struct blah blah;
    blah.x = "za wolnosc";
    blah.y = "nasza";
    blah.z = "i";
    blah.w = "wasza";
    printblah(blah);
    return 0;
}

