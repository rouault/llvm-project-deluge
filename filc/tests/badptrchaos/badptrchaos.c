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
        foo* h = head;
        f->next = h;
        head = f;
    }
    return NULL;
}

static void dump_array(const char* name, unsigned* array)
{
    printf("%s = ", name);
    unsigned i;
    for (i = 0; i < zlength(array); ++i) {
        if (i)
            printf(", ");
        printf("%u", array[i]);
    }
    printf("\n");
}

#define DUMP_ARRAY(array) dump_array(#array, array)

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
        ZASSERT(histogram[i] <= NTHREADS);

    //DUMP_ARRAY(histogram);
    
    return 0;
}

