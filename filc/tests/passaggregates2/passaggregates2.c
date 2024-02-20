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

void printfoo(int a, struct foo foo, char* b)
{
    zprintf("foo/a = %d\n", a);
    zprintf("foo.x = %d\n", foo.x);
    zprintf("foo.y = %d\n", foo.y);
    zprintf("foo/b = %s\n", b);
}

void printbar(char *a, struct bar bar, double b)
{
    zprintf("bar/a = %s\n", a);
    zprintf("bar.x = %lf\n", bar.x);
    zprintf("bar.y = %lf\n", bar.y);
    zprintf("bar/b = %lf\n", b);
}

void printbaz(float a, struct baz baz, char b)
{
    zprintf("baz/a = %lf\n", a);
    zprintf("baz.x = %s\n", baz.x);
    zprintf("baz.y = %s\n", baz.y);
    zprintf("baz/b = %c\n", b);
}

void printstuff(char* a, struct stuff stuff, char* b)
{
    zprintf("stuff/a = %s\n", a);
    zprintf("stuff.x = %d\n", stuff.x);
    zprintf("stuff.y = %d\n", stuff.y);
    zprintf("stuff.z = %d\n", stuff.z);
    zprintf("stuff.w = %d\n", stuff.w);
    zprintf("stuff/b = %s\n", b);
}

void printthingy(double a, struct thingy thingy, char* b)
{
    zprintf("thingy/a = %lf\n", a);
    zprintf("thingy.x = %lf\n", thingy.x);
    zprintf("thingy.y = %lf\n", thingy.y);
    zprintf("thingy.z = %lf\n", thingy.z);
    zprintf("thingy.w = %lf\n", thingy.w);
    zprintf("thingy/b = %s\n", b);
}

void printblah(char* a, struct blah blah, char* b)
{
    zprintf("blah/a = %s\n", a);
    zprintf("blah.x = %s\n", blah.x);
    zprintf("blah.y = %s\n", blah.y);
    zprintf("blah.z = %s\n", blah.z);
    zprintf("blah.w = %s\n", blah.w);
    zprintf("blah/b = %s\n", b);
}

int main(int argc, char** argv)
{
    struct foo foo;
    foo.x = 1;
    foo.y = 2;
    printfoo(42, foo, "yo");
    struct bar bar;
    bar.x = 1.5;
    bar.y = 2.3;
    printbar("spoko", bar, 3.14);
    struct baz baz;
    baz.x = "hello";
    baz.y = "world";
    printbaz(2.17, baz, 'F');
    struct stuff stuff;
    stuff.x = 3;
    stuff.y = 4;
    stuff.z = 5;
    stuff.w = 6;
    printstuff("bardzo", stuff, "dobrze");
    struct thingy thingy;
    thingy.x = 3.8;
    thingy.y = 4.1;
    thingy.z = 5.3;
    thingy.w = 6.6;
    printthingy(9.99, thingy, "wat");
    struct blah blah;
    blah.x = "za wolnosc";
    blah.y = "nasza";
    blah.z = "i";
    blah.w = "wasza";
    printblah("liberum", blah, "veto");
    return 0;
}

