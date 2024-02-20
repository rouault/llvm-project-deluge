#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include "filc_runtime.h"

extern char** environ;

void pizlonated_f___init_libc(PIZLONATED_SIGNATURE);
void pizlonated_f_main(PIZLONATED_SIGNATURE);
void pizlonated_f_exit(PIZLONATED_SIGNATURE);

struct init_libc_args {
    filc_ptr environ;
    filc_ptr program_name;
};

struct main_args {
    int argc;
    filc_ptr argv;
};

struct filc_type_template init_libc_args_type_template = {
    .size = sizeof(struct init_libc_args),
    .alignment = alignof(struct init_libc_args),
    .trailing_array = NULL,
    .word_types = {
        FILC_WORD_TYPES_PTR,
        FILC_WORD_TYPES_PTR
    }
};

static filc_type_template main_args_type_template = {
    .size = sizeof(struct main_args),
    .alignment = alignof(struct main_args),
    .trailing_array = NULL,
    .word_types = {
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPES_PTR
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
    filc_ptr pizlonated_argv;
    int index;
    size_t environ_size;
    filc_ptr* environ_ptr;
    const filc_type* init_libc_args_type;
    const filc_type* main_args_type;

    PAS_ASSERT(argc >= 1);

    filc_initialize();

    init_libc_args_type = filc_get_type(&init_libc_args_type_template);
    main_args_type = filc_get_type(&main_args_type_template);

    pizlonated_argv = filc_ptr_forge_with_size(
        filc_allocate_many(filc_get_heap(&filc_one_ptr_type), argc + 1),
        (argc + 1) * sizeof(filc_ptr),
        &filc_one_ptr_type);

    for (index = 0; index < argc; ++index) {
        filc_ptr arg;
        filc_ptr* arg_ptr;
        size_t size;
        arg_ptr = (filc_ptr*)filc_ptr_ptr(pizlonated_argv) + index;
        size = strlen(argv[index]) + 1;
        arg = filc_ptr_forge_with_size(filc_allocate_int(size, 1), size, &filc_int_type);
        filc_ptr_store(arg_ptr, arg);
        memcpy(filc_ptr_ptr(arg), argv[index], size);
    }

    filc_ptr_store((filc_ptr*)filc_ptr_ptr(pizlonated_argv) + argc, filc_ptr_forge_invalid(NULL));

    for (environ_size = 0; environ[environ_size]; ++environ_size);
    environ_size++;

    init_libc_args = filc_allocate_one(filc_get_heap(init_libc_args_type));
    environ_ptr = filc_allocate_many(filc_get_heap(&filc_one_ptr_type), environ_size);
    filc_ptr_store(
        &init_libc_args->environ,
        filc_ptr_forge_with_size(environ_ptr, sizeof(filc_ptr) * environ_size, &filc_one_ptr_type));
    filc_ptr_store(
        &init_libc_args->program_name, filc_ptr_load((filc_ptr*)filc_ptr_ptr(pizlonated_argv)));

    for (index = 0; index < environ_size; ++index) {
        filc_ptr* env_ptr = (filc_ptr*)filc_ptr_ptr(init_libc_args->environ) + index;
        if (index == environ_size - 1) {
            PAS_ASSERT(!environ[index]);
            filc_low_level_ptr_safe_bzero(env_ptr, sizeof(filc_ptr));
        } else {
            size_t size;
            char* env_copy;
            PAS_ASSERT(environ[index]);
            size = strlen(environ[index]) + 1;
            env_copy = (char*)filc_allocate_int(size, 1);
            filc_ptr_store(env_ptr, filc_ptr_forge_with_size(env_copy, size, &filc_int_type));
            memcpy(env_copy, environ[index], size);
        }
    }

    memset(u.return_buffer, 0, sizeof(u.return_buffer));
    pizlonated_f___init_libc(init_libc_args, init_libc_args + 1, init_libc_args_type,
                             u.return_buffer, u.return_buffer + 2, &filc_int_type);

    filc_run_deferred_global_ctors();

    main_args = filc_allocate_one(filc_get_heap(main_args_type));
    main_args->argc = argc;
    filc_ptr_store(&main_args->argv, pizlonated_argv);

    memset(u.return_buffer, 0, sizeof(u.return_buffer));
    pizlonated_f_main(main_args, main_args + 1, main_args_type,
                   u.return_buffer, u.return_buffer + 2, &filc_int_type);

    exit_args = filc_allocate_int(sizeof(int), 1);
    *exit_args = u.result;
    pizlonated_f_exit(exit_args, exit_args + 1, &filc_int_type,
                      u.return_buffer, u.return_buffer + 2, &filc_int_type);

    PAS_ASSERT(!"Should not get here");
    return 1;
}

