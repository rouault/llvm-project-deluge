#include <pthread.h>
#include <stdfil.h>

static const char* hello = "hello";

static void thread_main(void* arg)
{
    ZASSERT(arg == hello);
}

int main()
{
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, (void* (*)(void*))thread_main, hello));
    void* result = hello;
    ZASSERT(!pthread_join(t, &result));
    ZASSERT(!result);
    return 0;
}

