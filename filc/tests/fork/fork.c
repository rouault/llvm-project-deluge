#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdfil.h>
#include <string.h>
#include <errno.h>

int main()
{
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
    } else
        printf("Pozdrowienia od dzieci\n");
    
    return 0;
}

