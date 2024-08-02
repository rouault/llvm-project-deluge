#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdfil.h>

static void* memory1;
static void* memory2;
static const size_t size = 1024 * 1024;
static bool flag;

static void* thread_main(void* arg)
{
    flag = true;
    zfence();
    memset(memory1, 42, size);
    memset(memory2, 0, size);
    return NULL;
}

int main()
{
    memory1 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    memory2 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    pthread_t thread;
    pthread_create(&thread, NULL, thread_main, NULL);
    while (!flag)
        sched_yield();
    size_t count = 1000;
    while (count--)
        sched_yield();
    munmap(memory1, size);
    munmap(memory2, size);
    pthread_join(thread, NULL);
    ZASSERT(!"Should not have gotten here, but we did, because the race condition was favorable - whatever.");
    return 0;
}

