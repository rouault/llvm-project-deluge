#include <stdfil.h>

struct foo {
    void* a;
    int b;
    void* c;
    int d;
    void* e;
};

int main()
{
    char* p = (char*)zalloc(sizeof(struct foo) * 5);

    ZASSERT(zptrphase(p) == -1);
    ZASSERT(!zisint(p));
    ZASSERT(zisunset(p));
    ZASSERT(zptrphase(p + 7) == -1);
    ZASSERT(!zisint(p + 7));
    ZASSERT(zisunset(p + 7));
    ZASSERT(zptrphase(p + 16) == -1);
    ZASSERT(!zisint(p + 16));
    ZASSERT(zisunset(p + 16));
    ZASSERT(zptrphase(p + 58) == -1);
    ZASSERT(!zisint(p + 58));
    ZASSERT(zisunset(p + 58));
    ZASSERT(zptrphase(p + 68) == -1);
    ZASSERT(!zisint(p + 68));
    ZASSERT(zisunset(p + 68));
    ZASSERT(zptrphase(p + sizeof(struct foo) * 4 + 58) == -1);
    ZASSERT(!zisint(p + sizeof(struct foo) * 4 + 58));
    ZASSERT(zisunset(p + sizeof(struct foo) * 4 + 58));
    ZASSERT(zptrphase(p + sizeof(struct foo) * 4 + 68) == -1);
    ZASSERT(!zisint(p + sizeof(struct foo) * 4 + 68));
    ZASSERT(zisunset(p + sizeof(struct foo) * 4 + 68));

    unsigned index;
    struct foo* f = (struct foo*)p;
    for (index = 5; index--;) {
        f[index].a = 0;
        f[index].b = 0;
        f[index].c = 0;
        f[index].d = 0;
        f[index].e = 0;
    }
    
    ZASSERT(!zptrphase(p));
    ZASSERT(!zisint(p));
    ZASSERT(!zisunset(p));
    ZASSERT(zptrphase(p + 7) == 7);
    ZASSERT(!zisint(p + 7));
    ZASSERT(!zisunset(p + 7));
    ZASSERT(zptrphase(p + 16) == -1);
    ZASSERT(zisint(p + 16));
    ZASSERT(!zisunset(p + 16));
    ZASSERT(zptrphase(p + 58) == -1);
    ZASSERT(zisint(p + 58));
    ZASSERT(!zisunset(p + 58));
    ZASSERT(zptrphase(p + 68) == 4);
    ZASSERT(!zisint(p + 68));
    ZASSERT(!zisunset(p + 68));
    ZASSERT(zptrphase(p + sizeof(struct foo) * 4 + 58) == -1);
    ZASSERT(zisint(p + sizeof(struct foo) * 4 + 58));
    ZASSERT(!zisunset(p + sizeof(struct foo) * 4 + 58));
    ZASSERT(zptrphase(p + sizeof(struct foo) * 4 + 68) == 4);
    ZASSERT(!zisint(p + sizeof(struct foo) * 4 + 68));
    ZASSERT(!zisunset(p + sizeof(struct foo) * 4 + 68));
    return 0;
}



