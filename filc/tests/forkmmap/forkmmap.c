#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdfil.h>
#include <sys/wait.h>

#define ASSERT(exp) do { \
    if ((exp)) \
        break; \
    fprintf(stderr, "%s:%d: %s: assertion %s failed.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    abort(); \
} while (0)

struct shared {
    char buf[100];
    bool flag;
};

static struct shared* memory;

static void child(void)
{
    strcpy(memory->buf, "hello, world!\n");
    
    zstore_store_fence();
    memory->flag = true;

    munmap(memory, 16384);
}

static void parent(void)
{
    while (!memory->flag)
        zfence();
    zfence();
    ASSERT(!strcmp(memory->buf, "hello, world!\n"));
    printf("%s", memory->buf);

    munmap(memory, 16384);
}
      
int main()
{
    memory = mmap(NULL, 16384, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANON, -1, 0);
    ASSERT(memory);
    ASSERT(memory != (void*)(intptr_t)-1);
    
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

