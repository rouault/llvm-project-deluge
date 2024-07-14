#include <stdfil.h>
#include <pthread.h>
#include <stdlib.h>

static void* thread_main(void* arg)
{
    ZASSERT(zthread_self_id() == 2);
    return NULL;
}

int main()
{
    ZASSERT(zthread_self_id() == 1);
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, thread_main, NULL));
    ZASSERT(!pthread_join(t, NULL));
    return 0;
}

