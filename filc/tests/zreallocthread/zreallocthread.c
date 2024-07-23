#include <stdfil.h>
#include <pizlonated_runtime.h>
#include <stdlib.h>

void* thread_main(void* arg)
{
    ZASSERT(!arg);
    for (;;) zcompiler_fence();
    return NULL;
}

int main()
{
    void* thread = zthread_create(thread_main, NULL);
    zprintf("Spoko\n");
    zgc_realloc(thread, 0);
    zprintf("Nie dobrze\n");
    return 0;
}

