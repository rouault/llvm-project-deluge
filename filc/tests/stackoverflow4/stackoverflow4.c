#include <stdfil.h>
#include <pthread.h>
#include <stdlib.h>
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
    size_t num_threads = 10;
    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    size_t index;
    for (index = 0; index < num_threads; ++index)
        ZASSERT(!pthread_create(threads + index, NULL, thread_main, NULL));
    ZASSERT(!pthread_join(threads[0], NULL));
    ZASSERT(!"Should never get here");
    return 0;
}
