#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdfil.h>
#include <stdbool.h>
#include <unistd.h>
#include "utils.h"

static bool done = false;

static void* thread_main(void* arg)
{
    ZASSERT(!arg);
    int* p = malloc(42);
    p[0] = 666;
    p[1] = 1410;
    opaque(NULL);
    printf("values: %d, %d\n", p[0], p[1]);
    done = true;
    while (!opaque(NULL));
    p[666] = 42;
    return NULL;
}

int main()
{
    pthread_t t;
    pthread_create(&t, NULL, thread_main, NULL);
    while (!done)
        zcompiler_fence();
    usleep(1000); /* Give the thread a chance too do the wrong thing. */
    return 0;
}
