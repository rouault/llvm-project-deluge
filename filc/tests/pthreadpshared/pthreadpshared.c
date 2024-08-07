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

struct shared {
    char buf[100];
    bool flag;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

static struct shared* memory;

static void child(void)
{
    strcpy(memory->buf, "hello, world!\n");

    pthread_mutex_lock(&memory->lock);
    memory->flag = true;
    pthread_cond_broadcast(&memory->cond);
    pthread_mutex_unlock(&memory->lock);

    munmap(memory, 16384);
}

static void parent(void)
{
    pthread_mutex_lock(&memory->lock);
    while (!memory->flag)
        pthread_cond_wait(&memory->cond, &memory->lock);
    pthread_mutex_unlock(&memory->lock);
    ASSERT(!strcmp(memory->buf, "hello, world!\n"));
    printf("%s", memory->buf);

    munmap(memory, 16384);
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
    pthread_condattr_t cond_attr;
    ASSERT(!pthread_condattr_init(&cond_attr));
    ASSERT(!pthread_condattr_setpshared(&cond_attr, 1));
    ASSERT(!pthread_cond_init(&memory->cond, &cond_attr));
    
    int fork_result = fork();
    ASSERT(fork_result >= 0);
    if (!fork_result)
        child();
    else {
        parent();
        int status;
        int wait_result = wait(&status);
        ASSERT(wait_result == fork_result);
        ASSERT(WIFEXITED(status));
        ASSERT(!WEXITSTATUS(status));
    }
    return 0;
}

