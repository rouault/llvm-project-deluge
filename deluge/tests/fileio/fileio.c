#include <stdfil.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    int fd = zsys_open("deluge/tests/fileio/test.txt", 0);
    ZASSERT(fd > 2);

    char buf[100];
    __builtin_memset(buf, 0, sizeof(buf));
    int result = zsys_read(fd, buf, sizeof(buf));
    ZASSERT(result == 14);
    zprintf("read the zsys way: %s", buf);
    zsys_close(fd);

    fd = open("deluge/tests/fileio/test.txt", O_RDONLY);
    ZASSERT(fd > 2);
    __builtin_memset(buf, 0, sizeof(buf));
    result = read(fd, buf, sizeof(buf));
    ZASSERT(result == 14);
    zprintf("read with read way: %s", buf);
    close(fd);

    FILE* fin = fopen("deluge/tests/fileio/test.txt", "r");
    ZASSERT(fin);
    __builtin_memset(buf, 0, sizeof(buf));
    result = fread(buf, 1, sizeof(buf), fin);
    ZASSERT(result == 14);
    printf("read the fread way: %s", buf);
    fclose(fin);
    
    return 0;
}


