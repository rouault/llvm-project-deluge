#include <stdfil.h>

int main(int argc, char** argv)
{
    void* mutex = zthread_mutex_create();
    ZASSERT(mutex);
    ZASSERT(zthread_mutex_lock(mutex));
    ZASSERT(zthread_mutex_unlock(mutex));
    ZASSERT(zthread_mutex_trylock(mutex));
    ZASSERT(zthread_mutex_unlock(mutex));
    zthread_mutex_delete(mutex);
    ZASSERT(zthread_mutex_lock(mutex));
    ZASSERT(zthread_mutex_unlock(mutex));
    ZASSERT(zthread_mutex_trylock(mutex));
    ZASSERT(zthread_mutex_unlock(mutex));
    zprintf("dobrze\n");
    return 0;
}

