#include <pthread.h>
#include <stdfil.h>

static const char* hello = "hello";
static const char* world = "world";

static void* thread_main(void* arg)
{
    ZASSERT(arg == hello);
    return world;
}

int main()
{
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, thread_main, hello));
    void* result = hello;
    ZASSERT(!pthread_join(t, &result));
    ZASSERT(result == world);
    return 0;
}

