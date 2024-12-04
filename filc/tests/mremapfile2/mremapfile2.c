#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>

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

static size_t readloop(int fd, char* data, size_t size)
{
    size_t bytes_read = 0;
    while (size) {
        ssize_t result = read(fd, data, size);
        if (!result)
            return bytes_read;
        if (result == -1) {
            ASSERT(errno == EINTR);
            continue;
        }
        ASSERT(result > 0);
        ASSERT(result <= size);
        data += result;
        size -= result;
        bytes_read += result;
    }
    return bytes_read;
}

int main()
{
    unlink("filc/test-output/mremapfile2/mmapfile.dat");
    int fd = open("filc/test-output/mremapfile2/mmapfile.dat", O_CREAT | O_RDWR, 0644);
    ASSERT(fd > 2);

    unsigned i;
    for (i = 0; i < 16384; ++i) {
        char c = (char)i;
        writeloop(fd, &c, 1);
    }
        
    void* memory = mmap(NULL, 4096, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    ASSERT(memory);
    ASSERT(memory != (void*)(intptr_t)-1);

    for (i = 0; i < 4096; ++i)
        ASSERT(((char*)memory)[i] == (char)i);

    strcpy(memory, "hello, world!\n");

    /* Test the shrinking in place case. */
    void* memory2 = mremap(memory, 4096, 16384, MREMAP_MAYMOVE);
    ASSERT(memory2);
    ASSERT(memory2 != memory);
    ASSERT(memory2 != (void*)(long)-1);

    for (i = 0; i < 4096; ++i) {
        char c = ((char*)memory2)[i];
        char expect_c;
        if (i <= strlen("hello, world!\n"))
            expect_c = "hello, world!\n"[i];
        else
            expect_c = (char)i;
        if (c != expect_c)
            printf("At index = %u, expected_c = %d, got c = %d.\n", i, expect_c, c);
        ASSERT(c == expect_c);
    }

    for (i = 4096; i < 16384; ++i)
        ((char*)memory2)[i] = (char)(i * 3);

    munmap(memory2, 16384);
    close(fd);

    fd = open("filc/test-output/mremapfile2/mmapfile.dat", O_RDONLY);
    ASSERT(fd > 2);
    for (i = 0; i < 16384; ++i) {
        char c;
        ASSERT(readloop(fd, &c, 1) == 1);
        char expect_c;
        if (i <= strlen("hello, world!\n"))
            expect_c = "hello, world!\n"[i];
        else if (i < 4096)
            expect_c = (char)i;
        else
            expect_c = (char)(i * 3);
        if (c != expect_c)
            printf("At index = %u, expected_c = %d, got c = %d.\n", i, expect_c, c);
        ASSERT(c == expect_c);
    }
    char c;
    ASSERT(readloop(fd, &c, 1) == 0);

    return 0;
}

