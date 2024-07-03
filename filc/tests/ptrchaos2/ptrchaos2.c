#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>

#define NTHREADS 10
#define REPEAT 30000
#define MAXLENGTH 5
#define MAXVAL 7
#define MAXIMUM (NTHREADS * REPEAT * MAXLENGTH * MAXVAL)

struct foo;
typedef struct foo foo;

struct foo {
    foo* next;
    unsigned value[1];
};

static foo* head;

static void* thread_main(void* arg)
{
    unsigned i;
    for (i = REPEAT; i--;) {
        unsigned length = (i % MAXLENGTH) + 1;
        foo* f = zgc_alloc(__builtin_offsetof(foo, value) + length * sizeof(unsigned));
        unsigned j;
        for (j = length; j--;)
            f->value[j] = (i + j) % (MAXVAL + 1);
        foo* h = head;
        f->next = h;
        head = f;
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

    foo* f;
    unsigned result = 0;
    unsigned count = 0;
    for (f = head; f; f = f->next) {
        count++;
        ZASSERT(zlength(f->value));
        for (i = zlength(f->value); i--;)
            result += f->value[i];
    }
    printf("count = %u\n", count);
    printf("result = %u\n", result);
    ZASSERT(result);
    ZASSERT(result <= MAXIMUM);
    
    return 0;
}

