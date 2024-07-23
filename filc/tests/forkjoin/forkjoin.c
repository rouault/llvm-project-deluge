#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pizlonated_runtime.h>
#include <stdbool.h>

#define ZASSERT(exp) do { \
    if ((exp)) \
        break; \
    fprintf(stderr, "%s:%d: %s: assertion %s failed.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    abort(); \
} while (0)

void* thread_main(void* arg)
{
    ZASSERT(!arg);
    for (;;) sched_yield();
    return NULL;
}

int main()
{
    void* thread = zthread_create(thread_main, NULL);
    int fork_result = fork();
    ZASSERT(fork_result >= 0);
    if (fork_result) {
        printf("Pozdrowienia od rodzicow\n");
        int status;
        int wait_result = wait(&status);
        if (wait_result < 0)
            printf("wait error: %s\n", strerror(errno));
        ZASSERT(wait_result > 0);
        ZASSERT(WIFEXITED(status));
        ZASSERT(!WEXITSTATUS(status));
    } else {
        bool result = zthread_join(thread, NULL);
        int my_errno = errno;
        printf("join result = %d, %s\n", result, strerror(my_errno));
        ZASSERT(!result);
        ZASSERT(my_errno == ESRCH);
        printf("Pozdrowienia od dzieci\n");
    }
    
    return 0;
}


