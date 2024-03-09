#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

static void child(int sockfd)
{
    int socks[2];
    ZASSERT(!socketpair(AF_UNIX, SOCK_STREAM, 0, socks));

    struct msghdr msg;
    union {
        struct cmsghdr hdr;
        char buf[CMSG_SPACE(sizeof(int))];
    } cmsgbuf;
    struct cmsghdr* cmsg;

    memset(&msg, 0, sizeof(msg));
    memset(&cmsgbuf, 0, sizeof(cmsgbuf));
    msg.msg_control = cmsgbuf.buf;
    msg.msg_controllen = sizeof(cmsgbuf.buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int*)CMSG_DATA(cmsg) = socks[0];

    char thingy = (char)42;
    
    struct iovec vec;
    vec.iov_base = &thingy;
    vec.iov_len = 1;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;

    printf("About to sendmsg\n");

    for (;;) {
        ssize_t send_result = sendmsg(sockfd, &msg, 0);
        int my_errno = errno;
        if (send_result == 1)
            break;
        ZASSERT(send_result == -1);
        ZASSERT(my_errno == EINTR);
    }

    ZASSERT(write(socks[1], "hello", strlen("hello") + 1) == strlen("hello") + 1);
}

static void parent(int sockfd)
{
    struct msghdr msg;
    union {
        struct cmsghdr hdr;
        char buf[CMSG_SPACE(sizeof(int))];
    } cmsgbuf;
    struct cmsghdr* cmsg;
    struct iovec vec;
    char thingy;

    memset(&msg, 0, sizeof(msg));
    vec.iov_base = &thingy;
    vec.iov_len = 1;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    memset(&cmsgbuf, 0, sizeof(cmsgbuf));
    msg.msg_control = &cmsgbuf.buf;
    msg.msg_controllen = sizeof(cmsgbuf.buf);

    for (;;) {
        ssize_t recv_result = recvmsg(sockfd, &msg, 0);
        int my_errno = errno;
        if (recv_result == 1)
            break;
        printf("recv_result = %ld, error = %s\n", (long)recv_result, strerror(my_errno));
        ZASSERT(recv_result == -1);
        ZASSERT(my_errno == EINTR);
    }

    ZASSERT(thingy == (char)42);

    cmsg = CMSG_FIRSTHDR(&msg);
    ZASSERT(cmsg);
    ZASSERT(cmsg->cmsg_type == SCM_RIGHTS);
    ZASSERT(cmsg->cmsg_level == SOL_SOCKET);
    ZASSERT(cmsg->cmsg_len = CMSG_LEN(sizeof(int)));
    int othersockfd = *(int*)CMSG_DATA(cmsg);

    char buf[100];
    ZASSERT(read(othersockfd, buf, sizeof(buf)) == strlen("hello") + 1);
    ZASSERT(!strcmp(buf, "hello"));
}

int main()
{
    int socks[2];
    ZASSERT(!socketpair(AF_UNIX, SOCK_STREAM, 0, socks));

    int result = fork();
    ZASSERT(result >= 0);
    if (!result) {
        printf("Running child!\n");
        close(socks[1]);
        child(socks[0]);
        printf("Child OK\n");
    } else {
        printf("Running parent!\n");
        close(socks[0]);
        parent(socks[1]);
        wait(NULL);
        printf("Parent OK\n");
    }
    return 0;
}

