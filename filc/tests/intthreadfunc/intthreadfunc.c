#include <pthread.h>
#include <stdfil.h>

static const char* hello = "hello";

static int thread_main(void* arg)
{
    ZASSERT(arg == hello);
    return 666;
}

int main()
{
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, (void* (*)(void*))thread_main, hello));
    void* result = hello;
    ZASSERT(!pthread_join(t, &result));
    zprintf("wtf\n");
    ZASSERT(result == (void*)666);
    return 0;
}

