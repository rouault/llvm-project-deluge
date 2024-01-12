#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>

int main(int argc, char** argv)
{
    pthread_rwlock_t rwlock;
    ZASSERT(!pthread_rwlock_init(&rwlock, NULL));
    ZASSERT(!pthread_rwlock_rdlock(&rwlock));
    ZASSERT(!pthread_rwlock_unlock(&rwlock));
    ZASSERT(!pthread_rwlock_tryrdlock(&rwlock));
    ZASSERT(!pthread_rwlock_unlock(&rwlock));
    ZASSERT(!pthread_rwlock_wrlock(&rwlock));
    ZASSERT(!pthread_rwlock_unlock(&rwlock));
    ZASSERT(!pthread_rwlock_trywrlock(&rwlock));
    ZASSERT(!pthread_rwlock_unlock(&rwlock));
    ZASSERT(!pthread_rwlock_destroy(&rwlock));
    printf("Wporzo\n");
    return 0;
}
