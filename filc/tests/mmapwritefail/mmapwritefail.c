#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdfil.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

static void* memory1;
static const size_t size = 1024 * 1024;
static bool flag;

static void writeloop(int fd, char* data, size_t size)
{
    while (size) {
        ssize_t result = write(fd, data, size);
        zprintf("Wrote %ld bytes.\n", result);
        ZASSERT(result);
        if (result == -1) {
            ZASSERT(errno == EINTR);
            continue;
        }
        ZASSERT(result > 0);
        ZASSERT(result <= size);
        data += result;
        size -= result;
    }
}

static void* thread_main(void* arg)
{
    flag = true;
    zfence();
    int fd = open("filc/tests/mmapwritefail/testfile.dat", O_CREAT | O_WRONLY, 0644);
    writeloop(fd, memory1, size);
    return NULL;
}

int main()
{
    memory1 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    pthread_t thread;
    pthread_create(&thread, NULL, thread_main, NULL);
    while (!flag)
        sched_yield();
    size_t count = 1000;
    while (count--)
        sched_yield();
    munmap(memory1, size);
    pthread_join(thread, NULL);
    ZASSERT(!"Should not have gotten here, but we did, because the race condition was favorable - whatever.");
    return 0;
}

