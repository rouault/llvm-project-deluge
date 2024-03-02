#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main()
{
    int fork_result = fork();
    if (fork_result) {
        printf("Pozdrowienia od rodzicow\n");
        wait(NULL);
    } else
        printf("Pozdrowienia od dzieci\n");
    
    return 0;
}

