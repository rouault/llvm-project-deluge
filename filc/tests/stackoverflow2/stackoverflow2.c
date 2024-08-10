#include <stdfil.h>
#include <pthread.h>
#include "utils.h"

static void foo(void)
{
    void (*recurse)(void) = opaque(foo);
    recurse();
}

static void* thread_main(void* arg)
{
    ZASSERT(!arg);
    foo();
    ZASSERT(!"Should not get here");
    return NULL;
}

int main()
{
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, thread_main, NULL));
    ZASSERT(!pthread_join(t, NULL));
    ZASSERT(!"Should never get here");
    return 0;
}
