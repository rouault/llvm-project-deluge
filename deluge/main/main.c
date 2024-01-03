#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include "deluge_runtime.h"

void deluded_f_main(DELUDED_SIGNATURE);

struct main_args {
    int argc;
    deluge_ptr argv;
};

static deluge_type ptr_type = {
    .size = sizeof(deluge_ptr),
    .alignment = alignof(deluge_ptr),
    .trailing_array = NULL,
    .word_types = {
        DELUGE_WORD_TYPE_PTR_PART1,
        DELUGE_WORD_TYPE_PTR_PART2,
        DELUGE_WORD_TYPE_PTR_PART3,
        DELUGE_WORD_TYPE_PTR_PART4
    }
};

static deluge_type main_args_type = {
    .size = sizeof(struct main_args),
    .alignment = alignof(struct main_args),
    .trailing_array = NULL,
    .word_types = {
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPE_PTR_PART1,
        DELUGE_WORD_TYPE_PTR_PART2,
        DELUGE_WORD_TYPE_PTR_PART3,
        DELUGE_WORD_TYPE_PTR_PART4
    }
};

int main(int argc, char** argv)
{
    union {
        uintptr_t return_buffer[2];
        int result;
    } u;
    struct main_args* args;
    int index;
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    args = deluge_allocate_one(deluge_get_heap(&main_args_type));
    args->argc = argc;
    args->argv.ptr = deluge_allocate_many(deluge_get_heap(&ptr_type), argc);
    args->argv.lower = args->argv.ptr;
    args->argv.upper = (deluge_ptr*)args->argv.ptr + argc;
    args->argv.type = &ptr_type;

    for (index = 0; index < argc; ++index) {
        deluge_ptr* arg_ptr = (deluge_ptr*)args->argv.ptr + index;
        size_t size = strlen(argv[index]) + 1;
        arg_ptr->ptr = deluge_allocate_int(size);
        arg_ptr->lower = arg_ptr->ptr;
        arg_ptr->upper = (char*)arg_ptr->ptr + size;
        arg_ptr->type = &deluge_int_type;
        memcpy(arg_ptr->ptr, argv[index], size);
    }

    deluded_f_main(args, args + 1, &main_args_type, u.return_buffer, u.return_buffer + 1, &deluge_int_type);

    return u.result;
}

