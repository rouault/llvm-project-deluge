#include <stdlib.h>
#include <stdfil.h>
#include <string.h>
#include <pthread.h>

#define SIZE 10000000
#define REPEAT 10

static char** one;
static char** two;

static void do_test(void)
{
    one = (char**)malloc(sizeof(char*) * SIZE);
    two = (char**)malloc(sizeof(char*) * SIZE);

    unsigned i;
    for (i = SIZE; i--;)
        one[i] = strdup("hello");

    zprintf("one[0] = %s\n", one[0]);

    memmove(two, one, sizeof(char*) * SIZE);
    
    free(one);
    zgc_request_and_wait();

    for (i = SIZE; i--;) {
        if (strcmp(two[i], "hello")) {
            zprintf("two[%u] = %s\n", i, two[i]);
            ZASSERT(!strcmp(two[i], "hello"));
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

