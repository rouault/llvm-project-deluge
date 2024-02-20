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
    printf("Bardzo dobrze\n");
    return 0;
}

