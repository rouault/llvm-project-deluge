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
    zprintf("This type is %T or whatever.\n", zgettype(&whatever));
    zprintf("While this type is %T or whatever.\n", zgettype(&another));
    zprintf("This type is %T and stuff.\n", ztypeof(struct whatever));
    return 0;
}

