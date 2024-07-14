#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>

#define NTHREADS 10
#define REPEAT 30000

struct foo;
typedef struct foo foo;

struct foo {
    foo* next;
    unsigned value;
};

static foo* head;

static void* thread_main(void* arg)
{
    unsigned i;
    for (i = REPEAT; i--;) {
        foo* f = zgc_alloc(sizeof(foo));
        f->value = i;
        for (;;) {
            foo* h = head; 
            f->next = h;

            /* They really couldn't have come up with a worse API, could they? */
            foo* tmp = h;
            if (__c11_atomic_compare_exchange_weak((void*_Atomic*)&head, &tmp, f,
                                                   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
                break;
        }
    }
    return NULL;
}

int main()
{
    pthread_t threads[NTHREADS];
    unsigned i;

    for (i = NTHREADS; i--;)
        ZASSERT(!pthread_create(threads + i, NULL, thread_main, NULL));

    for (i = NTHREADS; i--;)
        ZASSERT(!pthread_join(threads[i], NULL));
    
    unsigned histogram[REPEAT];
    __builtin_memset(histogram, 0, sizeof(histogram));
    foo* f;
    for (f = head; f; f = f->next)
        histogram[f->value]++;

    for (i = REPEAT; i--;)
        ZASSERT(histogram[i] == NTHREADS);
    
    return 0;
}

