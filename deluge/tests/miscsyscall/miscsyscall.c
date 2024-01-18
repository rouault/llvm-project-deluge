#include <stdfil.h>
#include <unistd.h>
#include <time.h>

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
    
    zprintf("No worries.\n");
    return 0;
}

