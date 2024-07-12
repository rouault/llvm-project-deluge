#include <stdfil.h>
#include <pizlonated_runtime.h>

static const char* hello = "hello";

static void thread_main(void* arg)
{
    ZASSERT(arg == hello);
}

int main()
{
    void* t = zthread_create((void* (*)(void*))thread_main, hello);
    ZASSERT(t);
    void* result = hello;
    ZASSERT(zthread_join(t, &result));
    ZASSERT(!result);
    return 0;
}

