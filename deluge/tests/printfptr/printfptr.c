#include <stdfil.h>

struct whatever {
    int a;
    char* b;
    int c[666];
    char* d;
    int e[42];
    char* f;
    int g;
};

struct another {
    int a[666];
    char* b;
    int c[42];
    char* d;
    int e;
    char* f;
    int g;
};

int main()
{
    struct whatever whatever;
    struct another another;
    zprintf("This ptr is %P and stuff.\n", zunsafe_forge(0x12345, char, 666));
    zprintf("This type is %T or whatever.\n", zgettype(&whatever));
    zprintf("This ptr is %P and stuff and %P while this type is %T or whatever.\n",
            zunsafe_forge(0x666, double, 12345),
            zunsafe_forge(0x42, struct another, 0666),
            zgettype(&another));
    return 0;
}

