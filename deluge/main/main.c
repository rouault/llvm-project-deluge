#include <stdio.h>
#include <stdlib.h>
#include "deluge_runtime.h"

void deluded_f_main(DELUDED_SIGNATURE);

int main(int argc, char** argv)
{
    union {
        uintptr_t return_buffer[2];
        int result;
    } u;
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    deluded_f_main(NULL, NULL, NULL, u.return_buffer, u.return_buffer + 1, &deluge_int_type);

    return u.result;
}

