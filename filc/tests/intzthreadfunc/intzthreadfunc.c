#include <stdfil.h>
#include <pizlonated_runtime.h>

static const char* hello = "hello";

static int thread_main(void* arg)
{
    ZASSERT(arg == hello);
    return 666;
}

int main()
{
    void* t = zthread_create((void* (*)(void*))thread_main, hello);
    ZASSERT(t);
    void* result = hello;
    ZASSERT(zthread_join(t, &result));
    zprintf("wtf\n");
    ZASSERT(result == (void*)666);
    return 0;
}

