#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdfil.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

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
static int semid;

static void child(void)
{
    strcpy(memory->buf, "hello, world!\n");

    memory->flag = true;

    struct sembuf opbuf;
    opbuf.sem_num = 0;
    opbuf.sem_op = 1;
    opbuf.sem_flg = 0;
    ASSERT(!semop(semid, &opbuf, 1));
    
    ASSERT(!shmdt(memory));
}

static void parent(void)
{
    struct sembuf opbuf;
    opbuf.sem_num = 0;
    opbuf.sem_op = -1;
    opbuf.sem_flg = 0;
    ASSERT(!semop(semid, &opbuf, 1));
    ASSERT(memory->flag);
    ASSERT(!strcmp(memory->buf, "hello, world!\n"));
    printf("%s", memory->buf);

    ASSERT(!shmdt(memory));
}
      
int main()
{
    int shmid = shmget(IPC_PRIVATE, 16384, IPC_CREAT | IPC_EXCL | 0600);
    ASSERT(shmid >= 0);

    memory = shmat(shmid, NULL, 0);
    ASSERT(memory);
    ASSERT(memory != (void*)(intptr_t)-1);

    ASSERT(!shmctl(shmid, IPC_RMID, NULL));

    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0600);
    ASSERT(semid >= 0);
    
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
        ASSERT(!semctl(semid, 0, IPC_RMID));
    }
    return 0;
}

