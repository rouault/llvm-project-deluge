#include <stdfil.h>
#include <stdbool.h>

static const char* foo = "foo";

static void condition(void* arg)
{
    ZASSERT(arg == foo);
}

static void before_sleep(void* arg)
{
    ZASSERT(arg == foo);
    zprintf("wtf\n");
}

int main()
{
    int x;
    ZASSERT(zpark_if(&x, (bool (*)(void*))condition, before_sleep, foo, 1. / 0.)
            == zpark_condition_failed);
    return 0;
}
