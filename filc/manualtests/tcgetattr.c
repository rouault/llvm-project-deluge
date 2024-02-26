#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

#ifdef __PIZLONATOR_WAS_HERE__
#define c_ispeed __c_ispeed
#define c_ospeed __c_ospeed
#endif

#define FLAG(field, name) do { \
    if ((field) & (name)) \
        printf(" %s", #name); \
} while (0)

#define CC(name) printf("termios.c_cc[%s] = %u\n", #name, termios.c_cc[name])

static const char* b2s(speed_t baud)
{
#define BCASE(name) case name: return #name
    switch (baud) {
        BCASE(B0);
        BCASE(B50);
        BCASE(B75);
        BCASE(B110);
        BCASE(B134);
        BCASE(B150);
        BCASE(B200);
        BCASE(B300);
        BCASE(B600);
        BCASE(B1200);
        BCASE(B1800);
        BCASE(B2400);
        BCASE(B4800);
        BCASE(B9600);
        BCASE(B19200);
        BCASE(B38400);
        BCASE(B57600);
        BCASE(B115200);
        BCASE(B230400);
    default: {
        char buf[100];
        snprintf(buf, sizeof(buf), "bad baud %lu", (unsigned long)baud);
        return strdup(buf);
    } }
}

int main()
{
    struct termios termios;
    memset(&termios, 0, sizeof(termios));
    int result = tcgetattr(0, &termios);
    int my_errno = errno;
    printf("result = %d\n", result);
    if (result < 0)
        printf("error = %s\n", strerror(my_errno));
    printf("termios.c_iflag =");
    FLAG(termios.c_iflag, IGNBRK);
    FLAG(termios.c_iflag, BRKINT);
    FLAG(termios.c_iflag, IGNPAR);
    FLAG(termios.c_iflag, PARMRK);
    FLAG(termios.c_iflag, INPCK);
    FLAG(termios.c_iflag, ISTRIP);
    FLAG(termios.c_iflag, INLCR);
    FLAG(termios.c_iflag, IGNCR);
    FLAG(termios.c_iflag, ICRNL);
    FLAG(termios.c_iflag, IXON);
    FLAG(termios.c_iflag, IXANY);
    FLAG(termios.c_iflag, IXOFF);
    FLAG(termios.c_iflag, IMAXBEL);
    FLAG(termios.c_iflag, IUTF8);
    printf("\n");
    printf("termios.c_oflag =");
    FLAG(termios.c_oflag, OPOST);
    FLAG(termios.c_oflag, ONLCR);
    printf("\n");
    printf("termios.c_cflag =");
    if ((termios.c_cflag & CSIZE) == CS5)
        printf(" CS5");
    if ((termios.c_cflag & CSIZE) == CS6)
        printf(" CS6");
    if ((termios.c_cflag & CSIZE) == CS7)
        printf(" CS7");
    if ((termios.c_cflag & CSIZE) == CS8)
        printf(" CS8");
    FLAG(termios.c_cflag, CSTOPB);
    FLAG(termios.c_cflag, CREAD);
    FLAG(termios.c_cflag, PARENB);
    FLAG(termios.c_cflag, PARODD);
    FLAG(termios.c_cflag, HUPCL);
    FLAG(termios.c_cflag, CLOCAL);
    printf("\n");
    printf("termios.c_lflag =");
    FLAG(termios.c_lflag, ISIG);
    FLAG(termios.c_lflag, ICANON);
    FLAG(termios.c_lflag, ECHO);
    FLAG(termios.c_lflag, ECHOE);
    FLAG(termios.c_lflag, ECHOK);
    FLAG(termios.c_lflag, ECHONL);
    FLAG(termios.c_lflag, NOFLSH);
    FLAG(termios.c_lflag, TOSTOP);
    FLAG(termios.c_lflag, IEXTEN);
    FLAG(termios.c_lflag, ECHOCTL);
    FLAG(termios.c_lflag, ECHOPRT);
    FLAG(termios.c_lflag, ECHOKE);
    FLAG(termios.c_lflag, FLUSHO);
    FLAG(termios.c_lflag, PENDIN);
    FLAG(termios.c_lflag, EXTPROC);
    printf("\n");
    CC(VINTR);
    CC(VQUIT);
    CC(VERASE);
    CC(VKILL);
    CC(VEOF);
    CC(VTIME);
    CC(VMIN);
    CC(VSTART);
    CC(VSTOP);
    CC(VSUSP);
    CC(VEOL);
    CC(VREPRINT);
    CC(VDISCARD);
    CC(VWERASE);
    CC(VLNEXT);
    CC(VEOL2);
    printf("termios.c_ispeed = %s\n", b2s(termios.c_ispeed));
    printf("termios.c_ospeed = %s\n", b2s(termios.c_ospeed));
    return 0;
}

