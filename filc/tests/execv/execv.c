#include <unistd.h>
#include <stdfil.h>

int main()
{
    char* argv[3];
    argv[0] = "/bin/echo";
    argv[1] = "Witaj";
    argv[2] = NULL;
    execv("/bin/echo", argv);
    ZASSERT(!"Should not be reached");
    return 0;
}

