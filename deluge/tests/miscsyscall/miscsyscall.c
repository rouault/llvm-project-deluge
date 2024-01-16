#include <stdfil.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    unsigned zid;
    unsigned id;
    
    zid = zsys_getuid();
    id = getuid();
    ZASSERT(zid > 0);
    ZASSERT(zid == id);
    
    zid = zsys_geteuid();
    id = geteuid();
    ZASSERT(zid > 0);
    ZASSERT(zid == id);
    
    zid = zsys_getgid();
    id = getgid();
    ZASSERT(zid > 0);
    ZASSERT(zid == id);
    
    zprintf("No worries.\n");
    return 0;
}

