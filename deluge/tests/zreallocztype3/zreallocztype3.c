#include <stdfil.h>

struct foo {
    int a;
    char* b;
    int c;
    char* d;
    int e;
    char* f;
    int g;
    char* h;
    int i;
    char* j;
    int k;
    char* l;
    int m;
    char* n;
    int o;
    char* p;
    int q;
    char* r;
    int s;
    char* t;
    int u;
    char* v;
    int w;
    char* x;
    int y;
    char* z;
};

int main(int argc, char** argv)
{
    ztype* type = ztypeof(struct foo);
    zprintf("Spoko\n");
    zprintf("type = %p\n", type);
    zrealloc(type, char, 0);
    zprintf("Nie dobrze\n");
    return 0;
}

