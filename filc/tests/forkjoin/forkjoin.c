#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define ZASSERT(exp) do { \
    if ((exp)) \
        break; \
    fprintf(stderr, "%s:%d: %s: assertion %s failed.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
} while (0)

void* thread_main(void* arg)
{
    ZASSERT(!arg);
    for (;;) sched_yield();
    return NULL;
}

int main()
{
    pthread_t thread;
    pthread_create(&thread, NULL, thread_main, NULL);
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
        int result = pthread_join(thread, NULL);
        printf("join result = %d, %s\n", result, strerror(result));
        ZASSERT(result == ESRCH);
        printf("Pozdrowienia od dzieci\n");
    }
    
    return 0;
}


