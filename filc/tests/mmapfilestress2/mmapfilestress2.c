#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdfil.h>

#define ASSERT(exp) do { \
    if ((exp)) \
        break; \
    fprintf(stderr, "%s:%d: %s: assertion %s failed.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #exp); \
    abort(); \
} while (0)

static void writeloop(int fd, char* data, size_t size)
{
    while (size) {
        ssize_t result = write(fd, data, size);
        ASSERT(result);
        if (result == -1) {
            ASSERT(errno == EINTR);
            continue;
        }
        ASSERT(result > 0);
        ASSERT(result <= size);
        data += result;
        size -= result;
    }
}

int main()
{
    unlink("filc/test-output/mmapfilestress2/mmapfile.dat");
    int fd = open("filc/test-output/mmapfilestress2/mmapfile.dat", O_CREAT | O_RDWR, 0644);
    ASSERT(fd > 2);
    
    unsigned i;
    for (i = 0; i < 16384; ++i) {
        char c = (char)i;
        writeloop(fd, &c, 1);
    }
        
    unsigned count;
    for (count = 1000; count--;) {
        void* memory = mmap(NULL, 16384, PROT_READ, MAP_SHARED, fd, 0);
        ASSERT(memory);
        ASSERT(memory != (void*)(intptr_t)-1);

        for (i = 0; i < 16384; ++i) {
            char c = ((char*)memory)[i];
            char expect_c = (char)i;
            if (c != expect_c)
                printf("At index = %u, expected_c = %d, got c = %d.\n", i, expect_c, c);
            ASSERT(c == expect_c);
        }

        zgc_request_and_wait();
    }

    return 0;
}

