#include <stdfil.h>
#include <pizlonated_runtime.h>

static const char* hello = "hello";
static const char* world = "world";

static void* thread_main(void* arg)
{
    ZASSERT(arg == hello);
    return world;
}

int main()
{
    void* t = zthread_create(thread_main, hello);
    ZASSERT(t);
    void* result = hello;
    ZASSERT(zthread_join(t, &result));
    ZASSERT(result == world);
    return 0;
}

