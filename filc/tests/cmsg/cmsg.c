#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdfil.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

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

static size_t readloop(int fd, char* data, size_t size)
{
    size_t bytes_read = 0;
    while (size) {
        ssize_t result = read(fd, data, size);
        zprintf("Read %ld bytes.\n", result);
        if (!result)
            return bytes_read;
        if (result == -1) {
            ZASSERT(errno == EINTR);
            continue;
        }
        ZASSERT(result > 0);
        ZASSERT(result <= size);
        data += result;
        size -= result;
        bytes_read += result;
    }
    return bytes_read;
}

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
    zprintf("cmsg->cmsg_len = %u\n", (unsigned)cmsg->cmsg_len);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int*)CMSG_DATA(cmsg) = socks[0];

    char thingy = (char)42;
    
    struct iovec vec;
    vec.iov_base = &thingy;
    vec.iov_len = 1;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;

    zprintf("About to sendmsg. Sending fd = %d using fd = %d\n", socks[0], sockfd);

    for (;;) {
        ssize_t send_result = sendmsg(sockfd, &msg, 0);
        int my_errno = errno;
        if (send_result == 1)
            break;
        ZASSERT(send_result == -1);
        ZASSERT(my_errno == EINTR);
    }

    /* Gotta acknowledge that the other side got the file descriptor. If they get it after we exit,
       then they'll get a file descriptor that doesn't work. (Reproduced by running this test in
       legacy C on Darwin.) */
    char dummy;
    ZASSERT(readloop(sockfd, &dummy, 1) == 1);

    writeloop(socks[1], "hello", strlen("hello") + 1);
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
        zprintf("recv_result = %ld, error = %s\n", (long)recv_result, strerror(my_errno));
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
    zprintf("othersockfd = %d\n", othersockfd);

    char dummy;
    writeloop(sockfd, &dummy, 1);

    char buf[100];
    ZASSERT(readloop(othersockfd, buf, sizeof(buf)) == strlen("hello") + 1);
    ZASSERT(!strcmp(buf, "hello"));
}

int main()
{
    int socks[2];
    ZASSERT(!socketpair(AF_UNIX, SOCK_STREAM, 0, socks));

    int result = fork();
    ZASSERT(result >= 0);
    if (!result) {
        zprintf("Running child!\n");
        close(socks[1]);
        child(socks[0]);
        zprintf("Child OK\n");
    } else {
        zprintf("Running parent!\n");
        close(socks[0]);
        parent(socks[1]);
        wait(NULL);
        zprintf("Parent OK\n");
    }
    return 0;
}

