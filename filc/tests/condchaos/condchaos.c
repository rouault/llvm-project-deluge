#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>
#include <stdbool.h>

#define NTHREADS 10
#define REPEAT 10000

static pthread_mutex_t lock;
static pthread_cond_t cond;
static bool is_available;
static unsigned count;

static void* thread_main(void* arg)
{
    unsigned i;
    ZASSERT(!arg);
    for (i = REPEAT; i--;) {
        pthread_mutex_lock(&lock);
        while (!is_available)
            pthread_cond_wait(&cond, &lock);
        is_available = false;
        pthread_mutex_unlock(&lock);
        count++;
        pthread_mutex_lock(&lock);
        is_available = true;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main()
{
    pthread_t threads[NTHREADS];
    unsigned i;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    is_available = true;
    count = 0;

    zprintf("lock = %p, cond = %p\n", &lock, &cond);

    for (i = NTHREADS; i--;)
        ZASSERT(!pthread_create(threads + i, NULL, thread_main, NULL));

    for (i = NTHREADS; i--;)
        ZASSERT(!pthread_join(threads[i], NULL));
    
    printf("count = %u\n", count);
    return 0;
}

