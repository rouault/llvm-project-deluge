#include <pthread.h>
#include <stdfil.h>

int main()
{
    pthread_exit(NULL);
    ZASSERT(!"Should not be reached");
    return 1;
}
