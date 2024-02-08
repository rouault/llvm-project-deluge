#include <pthread.h>
#include <stdfil.h>

int main(int argc, char** argv)
{
    pthread_key_t specific;
    ZASSERT(!pthread_key_create(&specific, 0));
    zprintf("value before set: %p\n", pthread_getspecific(specific));
    ZASSERT(!pthread_setspecific(specific, (void*)666));
    zprintf("value after set: %p\n", pthread_getspecific(specific));
    ZASSERT(!pthread_key_delete(specific));
    zprintf("value after delete: %p\n", pthread_getspecific(specific));
    return 0;
}

