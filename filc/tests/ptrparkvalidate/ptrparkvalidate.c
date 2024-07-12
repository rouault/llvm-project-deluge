#include <stdfil.h>
#include <stdbool.h>

static const char* foo = "foo";

static void* condition(void* arg)
{
    ZASSERT(arg == foo);
    return "hello";
}

static void before_sleep(void* arg)
{
    ZASSERT(arg == foo);
    zprintf("wtf\n");
}

int main()
{
    int x;
    zpark_if(&x, (bool (*)(void*))condition, before_sleep, foo, 0.);
    zprintf("wtf\n");
    return 0;
}
