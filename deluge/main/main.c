#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include "deluge_runtime.h"

extern char** environ;

void deluded_f___init_libc(DELUDED_SIGNATURE);
void deluded_f_main(DELUDED_SIGNATURE);
void deluded_f_exit(DELUDED_SIGNATURE);

struct init_libc_args {
    deluge_ptr environ;
    deluge_ptr program_name;
};

struct main_args {
    int argc;
    deluge_ptr argv;
};

struct deluge_type_template init_libc_args_type_template = {
    .size = sizeof(struct init_libc_args),
    .alignment = alignof(struct init_libc_args),
    .trailing_array = NULL,
    .word_types = {
        DELUGE_WORD_TYPES_PTR,
        DELUGE_WORD_TYPES_PTR
    }
};

static deluge_type_template main_args_type_template = {
    .size = sizeof(struct main_args),
    .alignment = alignof(struct main_args),
    .trailing_array = NULL,
    .word_types = {
        DELUGE_WORD_TYPE_INT,
        DELUGE_WORD_TYPES_PTR
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
    int* exit_args;
    deluge_ptr deluded_argv;
    int index;
    size_t environ_size;
    deluge_ptr* environ_ptr;
    const deluge_type* init_libc_args_type;
    const deluge_type* main_args_type;

    PAS_ASSERT(argc >= 1);

    deluge_initialize();

    init_libc_args_type = deluge_get_type(&init_libc_args_type_template);
    main_args_type = deluge_get_type(&main_args_type_template);

    deluded_argv = deluge_ptr_forge_with_size(
        deluge_allocate_many(deluge_get_heap(&deluge_one_ptr_type), argc + 1),
        (argc + 1) * sizeof(deluge_ptr),
        &deluge_one_ptr_type);

    for (index = 0; index < argc; ++index) {
        deluge_ptr arg;
        deluge_ptr* arg_ptr;
        size_t size;
        arg_ptr = (deluge_ptr*)deluge_ptr_ptr(deluded_argv) + index;
        size = strlen(argv[index]) + 1;
        arg = deluge_ptr_forge_with_size(deluge_allocate_int(size, 1), size, &deluge_int_type);
        deluge_ptr_store(arg_ptr, arg);
        memcpy(deluge_ptr_ptr(arg), argv[index], size);
    }

    deluge_ptr_store((deluge_ptr*)deluge_ptr_ptr(deluded_argv) + argc, deluge_ptr_forge_invalid(NULL));

    for (environ_size = 0; environ[environ_size]; ++environ_size);
    environ_size++;

    init_libc_args = deluge_allocate_one(deluge_get_heap(init_libc_args_type));
    environ_ptr = deluge_allocate_many(deluge_get_heap(&deluge_one_ptr_type), environ_size);
    deluge_ptr_store(
        &init_libc_args->environ,
        deluge_ptr_forge_with_size(environ_ptr, sizeof(deluge_ptr) * environ_size, &deluge_one_ptr_type));
    deluge_ptr_store(
        &init_libc_args->program_name, deluge_ptr_load((deluge_ptr*)deluge_ptr_ptr(deluded_argv)));

    for (index = 0; index < environ_size; ++index) {
        deluge_ptr* env_ptr = (deluge_ptr*)deluge_ptr_ptr(init_libc_args->environ) + index;
        if (index == environ_size - 1) {
            PAS_ASSERT(!environ[index]);
            deluge_low_level_ptr_safe_bzero(env_ptr, sizeof(deluge_ptr));
        } else {
            size_t size;
            char* env_copy;
            PAS_ASSERT(environ[index]);
            size = strlen(environ[index]) + 1;
            env_copy = (char*)deluge_allocate_int(size, 1);
            deluge_ptr_store(env_ptr, deluge_ptr_forge_with_size(env_copy, size, &deluge_int_type));
            memcpy(env_copy, environ[index], size);
        }
    }

    memset(u.return_buffer, 0, sizeof(u.return_buffer));
    deluded_f___init_libc(init_libc_args, init_libc_args + 1, init_libc_args_type,
                          u.return_buffer, u.return_buffer + 2, &deluge_int_type);

    deluge_run_deferred_global_ctors();

    main_args = deluge_allocate_one(deluge_get_heap(main_args_type));
    main_args->argc = argc;
    deluge_ptr_store(&main_args->argv, deluded_argv);

    memset(u.return_buffer, 0, sizeof(u.return_buffer));
    deluded_f_main(main_args, main_args + 1, main_args_type,
                   u.return_buffer, u.return_buffer + 2, &deluge_int_type);

    exit_args = deluge_allocate_int(sizeof(int), 1);
    *exit_args = u.result;
    deluded_f_exit(exit_args, exit_args + 1, &deluge_int_type,
                   u.return_buffer, u.return_buffer + 2, &deluge_int_type);

    PAS_ASSERT(!"Should not get here");
    return 1;
}

