#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>
#include <semaphore.h>

#define NTHREADS 10
#define REPEAT 10000

typedef struct {
    const char* str;
} stuff;

static sem_t lock;
static unsigned count;

static void* thread_main(void* arg)
{
    unsigned i;
    for (i = REPEAT; i--;) {
        int value;
        ZASSERT(!sem_getvalue(&lock, &value));
        ZASSERT(!value || value == 1);
        ZASSERT(!sem_wait(&lock));
        ZASSERT(!sem_getvalue(&lock, &value));
        ZASSERT(!value);
        count++;
        ZASSERT(!sem_post(&lock));
    }
    stuff* s = (stuff*)arg;
    return (void*)s->str;
}

int main()
{
    pthread_t threads[NTHREADS];
    unsigned i;

    printf("lock is at %p\n", &lock);
    
    sem_init(&lock, 0, 1);

    for (i = NTHREADS; i--;) {
        stuff* s = zgc_alloc(sizeof(stuff));
        s->str = zasprintf("thread = %u", i);
        ZASSERT(!pthread_create(threads + i, NULL, thread_main, s));
    }

    for (i = NTHREADS; i--;) {
        const char* str;
        ZASSERT(!pthread_join(threads[i], (void**)&str));
        ZASSERT(!strcmp(str, zasprintf("thread = %u", i)));
    }
    
    printf("count = %u\n", count);
    return 0;
}

