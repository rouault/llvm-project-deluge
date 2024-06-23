#include <pthread.h>
#include <setjmp.h>
#include <stdfil.h>

jmp_buf jb;

static void* thread_main(void* arg)
{
    longjmp(jb, 1);
    return NULL;
}

int main()
{
    ZASSERT(!setjmp(jb));
    pthread_t t;
    pthread_create(&t, NULL, thread_main, NULL);
    pthread_join(t, NULL);
    ZASSERT(!"Should not get here");
    return 0;
}

