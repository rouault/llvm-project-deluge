#include <stdfil.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
    unsigned zid;
    unsigned id;
    int res;
    
    zid = zsys_getuid();
    id = getuid();
    ZASSERT((int)zid > 0);
    ZASSERT(zid == id);
    
    zid = zsys_geteuid();
    id = geteuid();
    ZASSERT((int)zid > 0);
    ZASSERT(zid == id);
    
    zid = zsys_getgid();
    id = getgid();
    ZASSERT((int)zid > 0);
    ZASSERT(zid == id);
    
    zid = zsys_getpid();
    id = getpid();
    ZASSERT((int)zid > 0);
    ZASSERT(zid == id);

    struct timespec ts;
    res = clock_gettime(CLOCK_REALTIME, &ts);
    ZASSERT(!res);
    ZASSERT(ts.tv_sec);

    struct stat st;
    zprintf("size = %zu\n", sizeof(st));
    zprintf("offset of nlink = %zu\n", __builtin_offsetof(struct stat, st_nlink));
    zprintf("sizeof nlink = %zu\n", sizeof(st.st_nlink));
    zprintf("sizeof(nlink_t) = %zu\n", sizeof(nlink_t));
    res = stat("deluge/tests/miscsyscall/testfile.txt", &st);
    ZASSERT(!res);
    ZASSERT(st.st_mode == 0644 | S_IFREG);
    ZASSERT(st.st_nlink == 1);
    ZASSERT(st.st_uid);
    ZASSERT(st.st_gid);
    ZASSERT(st.st_size == 86);
    
    zprintf("No worries.\n");
    return 0;
}

