#include <pthread.h>
#include <stdfil.h>

void* thread_main(void* arg)
{
    ZASSERT(!arg);
    for (;;) zcompiler_fence();
    return NULL;
}

int main()
{
    pthread_t thread;
    ZASSERT(!pthread_create(&thread, NULL, thread_main, NULL));
    zprintf("Spoko\n");
    zfree(thread);
    zprintf("Nie dobrze\n");
    return 0;
}

