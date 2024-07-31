#include <stdfil.h>
#include <stdbool.h>
#include <stdlib.h>

static char* callback(void)
{
    return "hello";
}

int main()
{
    zstack_scan((bool (*)(zstack_frame_description, void*))callback, NULL);
    return 0;
}

