#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdfil.h>
#include <sys/wait.h>
#include <pthread.h>

#define ASSERT(exp) do { \
    if ((exp)) \
        break; \
    fprintf(stderr, "%s:%d: %s: assertion %s failed.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    abort(); \
} while (0)

#define COUNT 10000

struct shared {
    unsigned count;
    pthread_mutex_t lock;
};

static struct shared* memory;

static void work(void)
{
    unsigned count;
    for (count = COUNT; count--;) {
        pthread_mutex_lock(&memory->lock);
        memory->count++;
        pthread_mutex_unlock(&memory->lock);
    }
}

int main()
{
    memory = mmap(NULL, 16384, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANON, -1, 0);
    ASSERT(memory);
    ASSERT(memory != (void*)(intptr_t)-1);

    pthread_mutexattr_t lock_attr;
    ASSERT(!pthread_mutexattr_init(&lock_attr));
    ASSERT(!pthread_mutexattr_setpshared(&lock_attr, 1));
    ASSERT(!pthread_mutex_init(&memory->lock, &lock_attr));
    
    int fork_result = fork();
    ASSERT(fork_result >= 0);
    if (!fork_result) {
        work();
        return 0;
    } else {
        work();
        int status;
        int wait_result = wait(&status);
        ASSERT(wait_result == fork_result);
        ASSERT(WIFEXITED(status));
        ASSERT(!WEXITSTATUS(status));
    }

    ASSERT(memory->count == COUNT * 2);
    
    return 0;
}

