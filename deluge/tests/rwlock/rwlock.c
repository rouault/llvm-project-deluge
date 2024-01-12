#include <stdfil.h>

int main(int argc, char** argv)
{
    void* rwlock = zthread_rwlock_create();
    ZASSERT(rwlock);
    ZASSERT(zthread_rwlock_rdlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    ZASSERT(zthread_rwlock_tryrdlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    ZASSERT(zthread_rwlock_wrlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    ZASSERT(zthread_rwlock_trywrlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    zthread_rwlock_delete(rwlock);
    ZASSERT(zthread_rwlock_rdlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    ZASSERT(zthread_rwlock_tryrdlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    ZASSERT(zthread_rwlock_wrlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    ZASSERT(zthread_rwlock_trywrlock(rwlock));
    ZASSERT(zthread_rwlock_unlock(rwlock));
    zprintf("Spoko\n");
    return 0;
}
