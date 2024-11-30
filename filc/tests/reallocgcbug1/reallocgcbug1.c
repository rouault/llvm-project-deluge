#include <stdlib.h>
#include <stdfil.h>
#include <string.h>
#include <pthread.h>

#define SIZE 10000000
#define REPEAT 10
#define NUM_YIELDS 1000

static char** one;
static char** two;

static void* thread_main(void* arg)
{
    unsigned i;
    for (i = NUM_YIELDS; i--;)
        sched_yield();
    free(one);
    zgc_request_and_wait();
    for (i = SIZE; i--;)
        strdup("world");
    return NULL;
}

static void do_test(void)
{
    one = (char**)malloc(sizeof(char*) * SIZE);
    two = NULL;

    unsigned i;
    for (i = SIZE; i--;)
        one[i] = strdup("hello");

    zprintf("one[0] = %s\n", one[0]);

    pthread_t t;
    pthread_create(&t, NULL, thread_main, NULL);

    two = realloc(one, sizeof(char*) * SIZE * 2);
    
    pthread_join(t, NULL);

    for (i = SIZE; i--;) {
        char* str = two[i];
        if (strcmp(str, "hello")) {
            zprintf("two[%u] = %O, %s\n", i, str, str);
            ZASSERT(!strcmp(str, "hello"));
        }
    }
}

int main()
{
    unsigned i;
    for (i = REPEAT; i--;)
        do_test();
    
    return 0;
}

