#include <stdfil.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    int fd = zsys_open("deluge/tests/fileio/test.txt", 0);
    ZASSERT(fd > 2);

    char buf[100];
    __builtin_memset(buf, 0, sizeof(buf));
    int result = zsys_read(fd, buf, sizeof(buf));
    ZASSERT(result == 14);
    zprintf("read the zsys way: %s", buf);

    FILE* fin = fopen("deluge/tests/fileio/test.txt", "r");
    ZASSERT(fin);
    __builtin_memset(buf, 0, sizeof(buf));
    result = fread(buf, 1, sizeof(buf), fin);
    ZASSERT(result == 14);
    printf("read the fread way: %s", buf);
    
    return 0;
}


