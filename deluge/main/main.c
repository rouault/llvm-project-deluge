#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include "deluge_runtime.h"

extern char** environ;

void deluded_f___init_libc(DELUDED_SIGNATURE);
void deluded_f_main(DELUDED_SIGNATURE);

struct init_libc_args {
    deluge_ptr environ;
    deluge_ptr program_name;
};

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

struct deluge_type init_libc_args_type = {
    .size = sizeof(struct init_libc_args),
    .alignment = alignof(struct init_libc_args),
    .trailing_array = NULL,
    .word_types = {
        DELUGE_WORD_TYPE_PTR_PART1,
        DELUGE_WORD_TYPE_PTR_PART2,
        DELUGE_WORD_TYPE_PTR_PART3,
        DELUGE_WORD_TYPE_PTR_PART4,
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
    struct init_libc_args* init_libc_args;
    struct main_args* main_args;
    deluge_ptr deluded_argv;
    int index;
    size_t environ_size;

    PAS_ASSERT(argc >= 1);

    deluded_argv.ptr = deluge_allocate_many(deluge_get_heap(&ptr_type), argc);
    deluded_argv.lower = deluded_argv.ptr;
    deluded_argv.upper = (deluge_ptr*)deluded_argv.ptr + argc;
    deluded_argv.type = &ptr_type;

    for (index = 0; index < argc; ++index) {
        deluge_ptr* arg_ptr = (deluge_ptr*)deluded_argv.ptr + index;
        size_t size = strlen(argv[index]) + 1;
        arg_ptr->ptr = deluge_allocate_int(size);
        arg_ptr->lower = arg_ptr->ptr;
        arg_ptr->upper = (char*)arg_ptr->ptr + size;
        arg_ptr->type = &deluge_int_type;
        memcpy(arg_ptr->ptr, argv[index], size);
    }

    for (environ_size = 0; environ[environ_size]; ++environ_size);
    environ_size++;

    init_libc_args = deluge_allocate_one(deluge_get_heap(&init_libc_args_type));
    init_libc_args->environ.ptr = deluge_allocate_many(deluge_get_heap(&ptr_type), environ_size);
    init_libc_args->environ.lower = init_libc_args->environ.ptr;
    init_libc_args->environ.upper = (deluge_ptr*)init_libc_args->environ.ptr + environ_size;
    init_libc_args->environ.type = &ptr_type;
    init_libc_args->program_name = ((deluge_ptr*)deluded_argv.ptr)[0];

    for (index = 0; index < environ_size; ++index) {
        deluge_ptr* env_ptr = (deluge_ptr*)init_libc_args->environ.ptr + index;
        if (index == environ_size - 1) {
            PAS_ASSERT(!environ[index]);
            memset(env_ptr, 0, sizeof(deluge_ptr));
        } else {
            size_t size;
            PAS_ASSERT(environ[index]);
            size = strlen(environ[index]) + 1;
            env_ptr->ptr = deluge_allocate_int(size);
            env_ptr->lower = env_ptr->ptr;
            env_ptr->upper = (char*)env_ptr->ptr + size;
            env_ptr->type = &deluge_int_type;
            memcpy(env_ptr->ptr, environ[index], size);
        }
    }

    memset(u.return_buffer, 0, sizeof(u.return_buffer));
    deluded_f___init_libc(init_libc_args, init_libc_args + 1, &init_libc_args_type,
                          u.return_buffer, u.return_buffer + 1, &deluge_int_type);

    main_args = deluge_allocate_one(deluge_get_heap(&main_args_type));
    main_args->argc = argc;
    main_args->argv = deluded_argv;

    memset(u.return_buffer, 0, sizeof(u.return_buffer));
    deluded_f_main(main_args, main_args + 1, &main_args_type,
                   u.return_buffer, u.return_buffer + 1, &deluge_int_type);

    return u.result;
}

