#include <pthread.h>
#include <string.h>
#include <stdfil.h>

static void foo(void)
{
    pthread_exit("hello");
    ZASSERT(!"Should not get here");
}

static void bar(void)
{
    foo();
    ZASSERT(!"Should not get here");
}

static void baz(void)
{
    bar();
    ZASSERT(!"Should not get here");
}

static void thingy(void)
{
    baz();
    ZASSERT(!"Should not get here");
}

static void stuff(void)
{
    thingy();
    ZASSERT(!"Should not get here");
}

static void* thread_main(void* arg)
{
    ZASSERT(arg == (void*)666);
    stuff();
    ZASSERT(!"Should not get here");
}

int main()
{
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, thread_main, (void*)666));
    char* result;
    ZASSERT(!pthread_join(t, (void**)&result));
    ZASSERT(!strcmp(result, "hello"));
    return 0;
}
