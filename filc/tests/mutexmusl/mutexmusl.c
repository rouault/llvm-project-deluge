#include <stdio.h>
#include <pthread.h>
#include <stdfil.h>

int main(int argc, char** argv)
{
    pthread_mutex_t mutex;
    ZASSERT(!pthread_mutex_init(&mutex, NULL));
    ZASSERT(!pthread_mutex_lock(&mutex));
    ZASSERT(!pthread_mutex_unlock(&mutex));
    ZASSERT(!pthread_mutex_trylock(&mutex));
    ZASSERT(!pthread_mutex_unlock(&mutex));
    ZASSERT(!pthread_mutex_destroy(&mutex));

    pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
    ZASSERT(!pthread_mutex_lock(&mutex2));
    ZASSERT(!pthread_mutex_unlock(&mutex2));
    ZASSERT(!pthread_mutex_trylock(&mutex2));
    ZASSERT(!pthread_mutex_unlock(&mutex2));
    ZASSERT(!pthread_mutex_destroy(&mutex2));

    pthread_mutexattr_t recursive_attr;
    ZASSERT(!pthread_mutexattr_init(&recursive_attr));
    ZASSERT(!pthread_mutexattr_settype(&recursive_attr, PTHREAD_MUTEX_RECURSIVE));
    pthread_mutex_t recursive_mutex;
    ZASSERT(!pthread_mutex_init(&recursive_mutex, &recursive_attr));
    size_t index;
    for (index = 100; index--;)
        ZASSERT(!pthread_mutex_lock(&recursive_mutex));
    for (index = 100; index--;)
        ZASSERT(!pthread_mutex_trylock(&recursive_mutex));
    for (index = 200; index--;)
        ZASSERT(!pthread_mutex_unlock(&recursive_mutex));
    for (index = 100; index--;)
        ZASSERT(!pthread_mutex_trylock(&recursive_mutex));
    for (index = 100; index--;)
        ZASSERT(!pthread_mutex_lock(&recursive_mutex));
    for (index = 200; index--;)
        ZASSERT(!pthread_mutex_unlock(&recursive_mutex));
    pthread_mutex_destroy(&recursive_mutex);

    printf("Bardzo dobrze\n");
    return 0;
}

