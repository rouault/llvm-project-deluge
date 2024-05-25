#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include "filc_runtime.h"

extern char** environ;

filc_ptr pizlonated___init_libc(filc_global_initialization_context*);
filc_ptr pizlonated_main(filc_global_initialization_context*);
filc_ptr pizlonated_exit(filc_global_initialization_context*);

struct init_libc_args {
    filc_ptr environ;
    filc_ptr program_name;
};

struct main_args {
    int argc;
    filc_ptr argv;
};

int main(int argc, char** argv)
{
    static const bool verbose = false;
    
    filc_return_buffer return_buffer;
    filc_ptr init_libc_args_ptr;
    struct init_libc_args* init_libc_args;
    filc_ptr main_args_ptr;
    struct main_args* main_args;
    filc_ptr exit_args_ptr;
    int* exit_args;
    filc_ptr pizlonated_argv;
    int index;
    int environ_size;
    filc_ptr environ_ptr;
    filc_ptr __init_libc_ptr;
    filc_ptr main_ptr;
    filc_ptr exit_ptr;

    PAS_ASSERT(argc >= 1);

    filc_initialize();
    filc_thread* my_thread = filc_get_my_thread();
    filc_enter(my_thread);

    static const filc_origin origin = {
        .filename = "<crt>",
        .function = "main",
        .line = 0,
        .column = 0
    };

    struct {
        FILC_FRAME_BODY;
    } actual_frame;
    pas_zero_memory(&actual_frame, sizeof(actual_frame));
    filc_frame* frame = (filc_frame*)&actual_frame;
    frame->origin = &origin;
    frame->num_objects = 0;
    filc_push_frame(my_thread, frame);

    filc_native_frame native_frame;
    filc_push_native_frame(my_thread, &native_frame);

    pizlonated_argv = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(filc_ptr) * (argc + 1)));

    for (index = 0; index < argc; ++index) {
        filc_ptr arg;
        filc_ptr arg_ptr;
        size_t size;
        arg_ptr = filc_ptr_with_offset(pizlonated_argv, index * sizeof(filc_ptr));
        size = strlen(argv[index]) + 1;
        arg = filc_ptr_create(my_thread, filc_allocate_int(my_thread, size));
        memcpy(filc_ptr_ptr(arg), argv[index], size);
        filc_check_write_ptr(arg_ptr, NULL);
        filc_ptr_store(my_thread, (filc_ptr*)filc_ptr_ptr(arg_ptr), arg);
    }

    filc_ptr arg_ptr = filc_ptr_with_offset(pizlonated_argv, argc * sizeof(filc_ptr));
    filc_check_write_ptr(arg_ptr, NULL);
    filc_ptr_store(my_thread, (filc_ptr*)filc_ptr_ptr(arg_ptr), filc_ptr_forge_null());

    for (environ_size = 0; environ[environ_size]; ++environ_size);
    environ_size++;

    init_libc_args_ptr = filc_ptr_create(
        my_thread, filc_allocate(my_thread, sizeof(struct init_libc_args)));
    FILC_CHECK_PTR_FIELD(init_libc_args_ptr, struct init_libc_args, environ, filc_write_access);
    FILC_CHECK_PTR_FIELD(init_libc_args_ptr, struct init_libc_args, program_name, filc_write_access);
    init_libc_args = (struct init_libc_args*)filc_ptr_ptr(init_libc_args_ptr);
    environ_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(filc_ptr) * environ_size));
    filc_ptr_store(my_thread, &init_libc_args->environ, environ_ptr);
    filc_ptr_store(
        my_thread, &init_libc_args->program_name,
        filc_ptr_load(my_thread, (filc_ptr*)filc_ptr_ptr(pizlonated_argv)));

    for (index = 0; index < environ_size; ++index) {
        filc_ptr env_ptr = filc_ptr_with_offset(environ_ptr, index * sizeof(filc_ptr));
        filc_ptr env_value;
        if (index == environ_size - 1) {
            PAS_ASSERT(!environ[index]);
            env_value = filc_ptr_forge_null();
        } else {
            size_t size;
            filc_ptr env_copy;
            PAS_ASSERT(environ[index]);
            size = strlen(environ[index]) + 1;
            env_copy = filc_ptr_create(my_thread, filc_allocate_int(my_thread, size));
            filc_ptr_store(my_thread, (filc_ptr*)filc_ptr_ptr(env_ptr), env_copy);
            memcpy(filc_ptr_ptr(env_copy), environ[index], size);
        }
    }

    __init_libc_ptr = pizlonated___init_libc(NULL);
    if (verbose) {
        pas_log("__init_libc object = %p\n", filc_ptr_object(__init_libc_ptr));
        pas_log("__init_libc object->lower = %p\n", filc_ptr_object(__init_libc_ptr)->lower);
        pas_log("__init_libc object->upper = %p\n", filc_ptr_object(__init_libc_ptr)->upper);
        pas_log("__init_libc ptr = %p\n", filc_ptr_ptr(__init_libc_ptr));
    }
    filc_thread_track_object(my_thread, filc_ptr_object(__init_libc_ptr));
    filc_check_function_call(__init_libc_ptr);
    filc_lock_top_native_frame(my_thread);
    ((void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(__init_libc_ptr))(
        my_thread, init_libc_args_ptr, filc_ptr_for_int_return_buffer(&return_buffer));
    filc_unlock_top_native_frame(my_thread);

    filc_run_deferred_global_ctors(my_thread);

    main_args_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(struct main_args)));
    FILC_CHECK_INT_FIELD(main_args_ptr, struct main_args, argc, filc_write_access);
    FILC_CHECK_PTR_FIELD(main_args_ptr, struct main_args, argv, filc_write_access);
    main_args = (struct main_args*)filc_ptr_ptr(main_args_ptr);
    main_args->argc = argc;
    filc_ptr_store(my_thread, &main_args->argv, pizlonated_argv);

    main_ptr = pizlonated_main(NULL);
    filc_thread_track_object(my_thread, filc_ptr_object(main_ptr));
    filc_check_function_call(main_ptr);
    filc_ptr rets = filc_ptr_for_int_return_buffer(&return_buffer);
    filc_lock_top_native_frame(my_thread);
    ((void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(main_ptr))(my_thread, main_args_ptr, rets);
    filc_unlock_top_native_frame(my_thread);

    if (verbose)
        pas_log("Exiting!\n");

    exit_args_ptr = filc_ptr_create(my_thread, filc_allocate_int(my_thread, sizeof(int)));
    exit_args = (int*)filc_ptr_ptr(exit_args_ptr);
    *exit_args = *(int*)filc_ptr_ptr(rets);
    exit_ptr = pizlonated_exit(NULL);
    filc_thread_track_object(my_thread, filc_ptr_object(exit_ptr));
    filc_lock_top_native_frame(my_thread);
    ((void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(exit_ptr))(
        my_thread, exit_args_ptr, filc_ptr_for_int_return_buffer(&return_buffer));
    filc_unlock_top_native_frame(my_thread);

    PAS_ASSERT(!"Should not get here");
    return 1;
}

