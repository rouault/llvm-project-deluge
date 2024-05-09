#include <pthread.h>
#include <signal.h>
#include <stdio.h>

static void dumpsigmask(const char* prefix, sigset_t* set)
{
    int sig;
    for (sig = 1; sig < 32; ++sig) {
        if (sigismember(set, sig))
            printf("%s: signal %d.\n", prefix, sig);
    }
}

static void* thread_main(void* arg)
{
    sigset_t set;
    pthread_sigmask(0, NULL, &set);
    dumpsigmask("thread blocked", &set);
    return NULL;
}

int main()
{
    pthread_t thread;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);
    dumpsigmask("initial set", &set);

    sigset_t oldset;
    pthread_sigmask(SIG_BLOCK, &set, &oldset);
    pthread_create(&thread, NULL, thread_main, NULL);
    pthread_join(thread, NULL);

    return 0;
}

