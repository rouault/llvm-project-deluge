#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>

#define NTHREADS 10
#define REPEAT 10000

typedef struct {
    const char* str;
} stuff;

static pthread_rwlock_t lock;
static unsigned count;

static void* thread_main(void* arg)
{
    unsigned i;
    for (i = REPEAT; i--;) {
        pthread_rwlock_wrlock(&lock);
        count++;
        pthread_rwlock_unlock(&lock);
    }
    return NULL;
}

int main()
{
    pthread_t threads[NTHREADS];
    unsigned i;

    printf("lock is at %p\n", &lock);
    
    pthread_mutex_init(&lock, NULL);

    for (i = NTHREADS; i--;)
        ZASSERT(!pthread_create(threads + i, NULL, thread_main, NULL));

    for (i = NTHREADS; i--;)
        ZASSERT(!pthread_join(threads[i], NULL));
    
    printf("count = %u\n", count);
    return 0;
}

