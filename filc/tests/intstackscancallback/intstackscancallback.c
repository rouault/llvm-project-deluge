#include <stdfil.h>
#include <stdbool.h>
#include <stdlib.h>

static int callback(void)
{
    return 666;
}

int main()
{
    zstack_scan((bool (*)(zstack_frame_description, void*))callback, NULL);
    return 0;
}

