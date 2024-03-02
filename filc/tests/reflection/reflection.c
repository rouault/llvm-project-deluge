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
    char* p = (char*)zalloc(struct foo, 5);
    ZASSERT(!zptrphase(p));
    ZASSERT(!zisint(p));
    ZASSERT(zptrphase(p + 7) == 7);
    ZASSERT(!zisint(p + 7));
    ZASSERT(zptrphase(p + 32) == -1);
    ZASSERT(zisint(p + 32));
    ZASSERT(zptrphase(p + 90) == -1);
    ZASSERT(zisint(p + 90));
    ZASSERT(zptrphase(p + 100) == 4);
    ZASSERT(!zisint(p + 100));
    ZASSERT(zptrphase(p + sizeof(struct foo) * 4 + 90) == -1);
    ZASSERT(zisint(p + sizeof(struct foo) * 4 + 90));
    ZASSERT(zptrphase(p + sizeof(struct foo) * 4 + 100) == 4);
    ZASSERT(!zisint(p + sizeof(struct foo) * 4 + 100));
    return 0;
}



