#include <stdfil.h>
#include "utils.h"
#include <stdlib.h>
#include <pizlonated_runtime.h>

static const char* hello = "hello";

static void* thread_main(void* arg)
{
    ZASSERT(arg == hello);
    return NULL;
}

int main()
{
    ((int (*)(void))opaque(zthread_create(thread_main, hello)))();
    zprintf("wtf\n");
    return 0;
}

