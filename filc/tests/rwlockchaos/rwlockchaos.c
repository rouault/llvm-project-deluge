#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define NWTHREADS 2
#define NRTHREADS 10
#define REPEAT 10000
#define NCOUNTS 10

static pthread_rwlock_t lock;
static unsigned count[NCOUNTS];
static bool keep_going;

static unsigned write_acquisitions[NWTHREADS];
static unsigned read_acquisitions[NRTHREADS];

static void* write_thread_main(void* arg)
{
    unsigned thread_index = (unsigned)arg;
    unsigned i;
    for (i = REPEAT; i--;) {
        pthread_rwlock_wrlock(&lock);
        write_acquisitions[thread_index]++;
        unsigned j;
        for (j = NCOUNTS; j--;)
            count[j]++;
        pthread_rwlock_unlock(&lock);
    }
    return NULL;
}

static void* read_thread_main(void* arg)
{
    unsigned thread_index = (unsigned)arg;
    while (keep_going) {
        pthread_rwlock_rdlock(&lock);
        read_acquisitions[thread_index]++;
        unsigned j;
        for (j = NCOUNTS; j--;)
            ZASSERT(count[0] == count[j]);
        pthread_rwlock_unlock(&lock);
        usleep(1); /* Musl's rwlock algorithm does not have any biasing for write locks, so it will
                      lead to writer starvation. So weird. */
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
    pthread_t write_threads[NWTHREADS];
    pthread_t read_threads[NRTHREADS];
    unsigned i;

    printf("lock is at %p\n", &lock);
    
    pthread_rwlock_init(&lock, NULL);
    keep_going = true;

    for (i = NRTHREADS; i--;)
        ZASSERT(!pthread_create(read_threads + i, NULL, read_thread_main, (void*)i));
    for (i = NWTHREADS; i--;)
        ZASSERT(!pthread_create(write_threads + i, NULL, write_thread_main, (void*)i));

    for (i = NWTHREADS; i--;)
        ZASSERT(!pthread_join(write_threads[i], NULL));
    keep_going = false;
    for (i = NRTHREADS; i--;)
        ZASSERT(!pthread_join(read_threads[i], NULL));
    
    DUMP_ARRAY(count);
    DUMP_ARRAY(write_acquisitions);
    DUMP_ARRAY(read_acquisitions);
    return 0;
}

