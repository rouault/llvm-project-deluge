#define _GNU_SOURCE

#include <stdfil.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static unsigned maintid;

static void* thread_main(void* arg)
{
    unsigned othertid = zthread_self_id();
    ZASSERT(othertid);
    ZASSERT(othertid != maintid);
    ZASSERT(othertid == gettid());
    return NULL;
}

int main()
{
    maintid = zthread_self_id();
    ZASSERT(maintid);
    ZASSERT(maintid == gettid());
    pthread_t t;
    ZASSERT(!pthread_create(&t, NULL, thread_main, NULL));
    ZASSERT(!pthread_join(t, NULL));
    return 0;
}

