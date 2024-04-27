#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int z = 666;

static __thread int x = 42;
static __thread int* y = &z;

static pthread_key_t key;

static __thread unsigned count = 0;

static void destructor(void* value)
{
    printf("(dtor%u) x = %d\n", ++count, x);
    pthread_setspecific(key, (void*)1);
}

static void* thread_main(void* arg)
{
    x = 555;
    printf("(thr) x = %d\n", x);

    pthread_key_create(&key, destructor);
    pthread_setspecific(key, (void*)1);

    return NULL;
}

int main()
{
    printf("(1) x = %d\n", x);
    printf("(1) y = %d\n", *y);

    x = 35;
    int* p = malloc(sizeof(int));
    *p = 36;
    y = p;
    
    printf("(2) x = %d\n", x);
    printf("(2) y = %d\n", *y);

    pthread_t thread;
    pthread_create(&thread, NULL, thread_main, NULL);
    pthread_join(thread, NULL);
    
    return 0;
}

