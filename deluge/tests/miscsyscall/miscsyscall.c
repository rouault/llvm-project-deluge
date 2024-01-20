#include <unistd.h>
#include <stdio.h>
#include <stdfil.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>

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
    ZASSERT(st.st_mode == (0644 | S_IFREG));
    ZASSERT(st.st_nlink == 1);
    ZASSERT(st.st_uid);
    ZASSERT(st.st_gid);
    ZASSERT(st.st_size == 86);

    ZASSERT(zthread_self());
    ZASSERT(zthread_self() == zthread_self());
    ZASSERT((void*)pthread_self() == zthread_self());

    ZASSERT(open("this/is/totally/not/going/to/exist", 666, 42) < 0);
    printf("the error was: %s\n", strerror(errno));

    int fd = open("deluge/tests/miscsyscall/testfile.txt", O_RDONLY);
    ZASSERT(fd > 2);
    struct stat st2;
    res = fstat(fd, &st2);
    ZASSERT(!res);
    ZASSERT(st2.st_mode == (0644 | S_IFREG));
    ZASSERT(st2.st_nlink == 1);
    ZASSERT(st2.st_uid == st.st_uid);
    ZASSERT(st2.st_gid == st.st_gid);
    ZASSERT(st2.st_size == 86);

    struct passwd* passwd = getpwuid(getuid());
    ZASSERT(passwd);
    ZASSERT(strlen(passwd->pw_name));
    ZASSERT(strlen(passwd->pw_passwd));
    ZASSERT(passwd->pw_uid == getuid());
    ZASSERT(passwd->pw_gid == getgid());
    ZASSERT(strlen(passwd->pw_gecos));
    ZASSERT(strlen(passwd->pw_dir));
    ZASSERT(strlen(passwd->pw_shell));

    char* name = strdup(passwd->pw_name);
    char* passwdd = strdup(passwd->pw_passwd);
    char* gecos = strdup(passwd->pw_gecos);
    char* dir = strdup(passwd->pw_dir);
    char* shell = strdup(passwd->pw_shell);
    
    struct passwd* passwd2 = getpwuid(getuid());
    ZASSERT(passwd2);
    ZASSERT(passwd2 == passwd);
    ZASSERT(!strcmp(passwd->pw_name, name));
    ZASSERT(!strcmp(passwd->pw_passwd, passwdd));
    ZASSERT(passwd->pw_uid == getuid());
    ZASSERT(passwd->pw_gid == getgid());
    ZASSERT(!strcmp(passwd->pw_gecos, gecos));
    ZASSERT(!strcmp(passwd->pw_dir, dir));
    ZASSERT(!strcmp(passwd->pw_shell, shell));
    
    zprintf("No worries.\n");
    return 0;
}

