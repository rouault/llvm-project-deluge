#include <stdfil.h>
#include <stdbool.h>
#include <stdlib.h>

static void callback(void)
{
}

int main()
{
    zstack_scan((bool (*)(zstack_frame_description, void*))callback, NULL);
    return 0;
}

