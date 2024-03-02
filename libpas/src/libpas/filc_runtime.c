#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_runtime.h"

#if PAS_ENABLE_FILC

#include "filc_cat_table.h"
#include "filc_hard_heap_config.h"
#include "filc_heap_config.h"
#include "filc_heap_innards.h"
#include "filc_heap_table.h"
#include "filc_parking_lot.h"
#include "filc_slice_table.h"
#include "filc_type_table.h"
#include "pas_deallocate.h"
#include "pas_get_allocation_size.h"
#include "pas_hashtable.h"
#include "pas_scavenger.h"
#include "pas_string_stream.h"
#include "pas_try_allocate.h"
#include "pas_try_allocate_array.h"
#include "pas_try_allocate_intrinsic.h"
#include "pas_uint64_hash_map.h"
#include "pas_utils.h"
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/wait.h>

const filc_type_template filc_int_type_template = {
    .size = 1,
    .alignment = 1,
    .trailing_array = NULL,
    .word_types = { FILC_WORD_TYPE_INT }
};

const filc_type filc_int_type = {
    .index = FILC_INT_TYPE_INDEX,
    .size = 1,
    .alignment = 1,
    .num_words = 1,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = { FILC_WORD_TYPE_INT }
};

const filc_type filc_one_ptr_type = {
    .index = FILC_ONE_PTR_TYPE_INDEX,
    .size = sizeof(filc_ptr),
    .alignment = alignof(filc_ptr),
    .num_words = sizeof(filc_ptr) / FILC_WORD_SIZE,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = { FILC_WORD_TYPES_PTR }
};

const filc_type filc_int_ptr_type = {
    .index = FILC_INT_PTR_TYPE_INDEX,
    .size = FILC_WORD_SIZE + sizeof(filc_ptr),
    .alignment = alignof(filc_ptr),
    .num_words = 1 + sizeof(filc_ptr) / FILC_WORD_SIZE,
    .u = {
        .trailing_array = NULL,
    },
    .word_types = {
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPES_PTR
    }
};

const filc_type filc_function_type = {
    .index = FILC_FUNCTION_TYPE_INDEX,
    .size = 0,
    .alignment = 0,
    .num_words = 0,
    .u = {
        .runtime_config = NULL,
    },
    .word_types = { }
};

const filc_type filc_type_type = {
    .index = FILC_TYPE_TYPE_INDEX,
    .size = 0,
    .alignment = 0,
    .num_words = 0,
    .u = {
        .runtime_config = NULL,
    },
    .word_types = { }
};

const filc_type** filc_type_array = NULL;
unsigned filc_type_array_size = 0;
unsigned filc_type_array_capacity = 0;

filc_type_table filc_type_table_instance = PAS_HASHTABLE_INITIALIZER;
filc_heap_table filc_normal_heap_table = PAS_HASHTABLE_INITIALIZER;
filc_heap_table filc_hard_heap_table = PAS_HASHTABLE_INITIALIZER;
filc_deep_slice_table filc_global_slice_table = PAS_HASHTABLE_INITIALIZER;
filc_deep_cat_table filc_global_cat_table = PAS_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable filc_fast_type_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable filc_fast_heap_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;
pas_lock_free_read_ptr_ptr_hashtable filc_fast_hard_heap_table =
    PAS_LOCK_FREE_READ_PTR_PTR_HASHTABLE_INITIALIZER;

PAS_DEFINE_LOCK(filc_type);
PAS_DEFINE_LOCK(filc_type_ops);
PAS_DEFINE_LOCK(filc_global_initialization);

static void* allocate_utility_for_allocation_config(
    size_t size, const char* name, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(name);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    return filc_allocate_utility(size);
}

static void* allocate_int_for_allocation_config(
    size_t size, const char* name, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(name);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    return filc_allocate_int(size, 1);
}

static void deallocate_for_allocation_config(
    void* ptr, size_t size, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(size);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    filc_deallocate_yolo(ptr);
}

static void initialize_utility_allocation_config(pas_allocation_config* allocation_config)
{
    allocation_config->allocate = allocate_utility_for_allocation_config;
    allocation_config->deallocate = deallocate_for_allocation_config;
    allocation_config->arg = NULL;
}

static void initialize_int_allocation_config(pas_allocation_config* allocation_config)
{
    allocation_config->allocate = allocate_int_for_allocation_config;
    allocation_config->deallocate = deallocate_for_allocation_config;
    allocation_config->arg = NULL;
}

typedef struct {
    filc_slice_table slice_table;
    filc_cat_table cat_table;
} type_tables;

static pthread_key_t type_tables_key;

static void type_tables_destroy(void* value)
{
    type_tables* tables;
    pas_allocation_config allocation_config;

    tables = (type_tables*)value;

    if (!tables)
        return;

    initialize_utility_allocation_config(&allocation_config);
    filc_slice_table_destruct(&tables->slice_table, &allocation_config);
    filc_cat_table_destruct(&tables->cat_table, &allocation_config);
    filc_deallocate_yolo(tables);
}

struct musl_passwd {
    filc_ptr pw_name;
    filc_ptr pw_passwd;
    unsigned pw_uid;
    unsigned pw_gid;
    filc_ptr pw_gecos;
    filc_ptr pw_dir;
    filc_ptr pw_shell;
};

static const filc_type musl_passwd_type = {
    .index = FILC_MUSL_PASSWD_TYPE_INDEX,
    .size = sizeof(struct musl_passwd),
    .alignment = alignof(struct musl_passwd),
    .num_words = sizeof(struct musl_passwd) / FILC_WORD_SIZE,
    .u = {
        .trailing_array = NULL
    },
    .word_types = {
        FILC_WORD_TYPES_PTR, /* pw_name */
        FILC_WORD_TYPES_PTR, /* pw_passwd */
        FILC_WORD_TYPE_INT, /* pw_uid, pw_gid */
        FILC_WORD_TYPES_PTR, /* pw_gecos */
        FILC_WORD_TYPES_PTR, /* pw_dir */
        FILC_WORD_TYPES_PTR /* pw_shell */
    }
};

static pas_heap_ref musl_passwd_heap = {
    .type = (const pas_heap_type*)&musl_passwd_type,
    .heap = NULL,
    .allocator_index = 0
};

static void musl_passwd_free_guts(struct musl_passwd* musl_passwd)
{
    filc_deallocate_safe(filc_ptr_load(&musl_passwd->pw_name));
    filc_deallocate_safe(filc_ptr_load(&musl_passwd->pw_passwd));
    filc_deallocate_safe(filc_ptr_load(&musl_passwd->pw_gecos));
    filc_deallocate_safe(filc_ptr_load(&musl_passwd->pw_dir));
    filc_deallocate_safe(filc_ptr_load(&musl_passwd->pw_shell));
}

static void musl_passwd_threadlocal_destructor(void* ptr)
{
    struct musl_passwd* musl_passwd = (struct musl_passwd*)ptr;
    musl_passwd_free_guts(musl_passwd);
    filc_deallocate_yolo(musl_passwd);
}

static pthread_key_t musl_passwd_threadlocal_key;

struct musl_sigset {
    unsigned long bits[128 / sizeof(long)];
};

struct musl_sigaction {
    filc_ptr sa_handler_ish;
    struct musl_sigset sa_mask;
    int sa_flags;
    filc_ptr sa_restorer; /* ignored */
};

static const filc_type musl_sigaction_type = {
    .index = FILC_MUSL_SIGACTION_TYPE_INDEX,
    .size = sizeof(struct musl_sigaction),
    .alignment = alignof(struct musl_sigaction),
    .num_words = sizeof(struct musl_sigaction) / FILC_WORD_SIZE,
    .u = {
        .trailing_array = NULL
    },
    .word_types = {
        FILC_WORD_TYPES_PTR, /* sa_handler_ish */
        FILC_WORD_TYPE_INT, /* sa_mask */
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT,
        FILC_WORD_TYPE_INT, /* sa_flags */
        FILC_WORD_TYPES_PTR /* sa_restorer */
    }
};

#define DEFINE_RUNTIME_CONFIG(name, type, fresh_memory_constructor)     \
    static void name ## _initialize_fresh_memory(void* begin, void* end) \
    { \
        PAS_TESTING_ASSERT(((char*)end - (char*)begin) >= (ptrdiff_t)sizeof(type)); \
        fresh_memory_constructor(begin); \
    } \
    \
    static pas_basic_heap_runtime_config name = { \
        .base = { \
            .sharing_mode = pas_do_not_share_pages, \
            .statically_allocated = false, \
            .is_part_of_heap = true, \
            .is_flex = false, \
            .directory_size_bound_for_partial_views = 0, \
            .directory_size_bound_for_baseline_allocators = PAS_TYPED_BOUND_FOR_BASELINE_ALLOCATORS, \
            .directory_size_bound_for_no_view_cache = PAS_TYPED_BOUND_FOR_NO_VIEW_CACHE, \
            .max_segregated_object_size = PAS_TYPED_MAX_SEGREGATED_OBJECT_SIZE, \
            .max_bitfit_object_size = 0, \
            .view_cache_capacity_for_object_size = pas_heap_runtime_config_zero_view_cache_capacity, \
            .initialize_fresh_memory = name ## _initialize_fresh_memory \
        }, \
        .page_caches = &filc_page_caches \
    }

static int from_musl_signum(int signum)
{
    switch (signum) {
    case 1: return SIGHUP;
    case 2: return SIGINT;
    case 3: return SIGQUIT;
    case 4: return SIGILL;
    case 5: return SIGTRAP;
    case 6: return SIGABRT;
    case 7: return SIGBUS;
    case 8: return SIGFPE;
    case 9: return SIGKILL;
    case 10: return SIGUSR1;
    case 11: return SIGSEGV;
    case 12: return SIGUSR2;
    case 13: return SIGPIPE;
    case 14: return SIGALRM;
    case 15: return SIGTERM;
    case 17: return SIGCHLD;
    case 18: return SIGCONT;
    case 19: return SIGSTOP;
    case 20: return SIGTSTP;
    case 21: return SIGTTIN;
    case 22: return SIGTTOU;
    case 23: return SIGURG;
    case 24: return SIGXCPU;
    case 25: return SIGXFSZ;
    case 26: return SIGVTALRM;
    case 27: return SIGPROF;
    case 28: return SIGWINCH;
    case 29: return SIGIO;
    case 31: return SIGSYS;
    default: return -1;
    }
}

static int to_musl_signum(int signum)
{
    switch (signum) {
    case SIGHUP: return 1;
    case SIGINT: return 2;
    case SIGQUIT: return 3;
    case SIGILL: return 4;
    case SIGTRAP: return 5;
    case SIGABRT: return 6;
    case SIGBUS: return 7;
    case SIGFPE: return 8;
    case SIGKILL: return 9;
    case SIGUSR1: return 10;
    case SIGSEGV: return 11;
    case SIGUSR2: return 12;
    case SIGPIPE: return 13;
    case SIGALRM: return 14;
    case SIGTERM: return 15;
    case SIGCHLD: return 17;
    case SIGCONT: return 18;
    case SIGSTOP: return 19;
    case SIGTSTP: return 20;
    case SIGTTIN: return 21;
    case SIGTTOU: return 22;
    case SIGURG: return 23;
    case SIGXCPU: return 24;
    case SIGXFSZ: return 25;
    case SIGVTALRM: return 26;
    case SIGPROF: return 27;
    case SIGWINCH: return 28;
    case SIGIO: return 29;
    case SIGSYS: return 31;
    default: PAS_ASSERT(!"Bad signal number"); return -1;
    }
}

#define MAX_MUSL_SIGNUM 31u

typedef struct {
    pas_lock lock;
    pthread_t thread; /* this becomes NULL the moment that we join or detach. */
    unsigned id; /* non-zero unique identifier for this thread. if this thread dies then some future
                    thread may get this id, but the ids are unique among active threads. */
    bool is_running;
    bool is_joining;
    void (*callback)(PIZLONATED_SIGNATURE);
    filc_ptr arg_ptr;
    filc_ptr result_ptr;
    filc_ptr cookie_ptr;

    bool have_deferred_signals;
    uint64_t num_deferred_signals[MAX_MUSL_SIGNUM + 1];
    bool in_filc;
} zthread;

static unsigned zthread_next_id = 1;

static void zthread_construct(zthread* thread)
{
    pas_lock_construct(&thread->lock);
    thread->thread = NULL;

    thread->is_running = false;
    thread->is_joining = false;
    thread->callback = NULL;

    thread->arg_ptr = filc_ptr_forge_invalid(NULL);
    thread->result_ptr = filc_ptr_forge_invalid(NULL);
    thread->cookie_ptr = filc_ptr_forge_invalid(NULL);

    /* I'm not sure if this needs to be a CAS loop. Maybe we're only called when libpas is holding
       "enough" global locks? */
    for (;;) {
        unsigned old_value = zthread_next_id;
        unsigned new_value;
        PAS_ASSERT(!pas_add_uint32_overflow(old_value, 1, &new_value));
        if (pas_compare_and_swap_uint32_weak(&zthread_next_id, old_value, new_value)) {
            thread->id = old_value;
            break;
        }
    }

    thread->have_deferred_signals = false;
    size_t index;
    for (index = MAX_MUSL_SIGNUM + 1; index--;)
        thread->num_deferred_signals[index] = 0;
    thread->in_filc = false;
}

DEFINE_RUNTIME_CONFIG(zthread_runtime_config, zthread, zthread_construct);

static const filc_type zthread_type = {
    .index = FILC_THREAD_TYPE_INDEX,
    .size = sizeof(zthread),
    .alignment = alignof(zthread),
    .num_words = 0,
    .u = {
        .runtime_config = &zthread_runtime_config
    },
    .word_types = { }
};

static pas_heap_ref zthread_heap = {
    .type = (const pas_heap_type*)&zthread_type,
    .heap = NULL,
    .allocator_index = 0
};

static bool should_destroy_thread(zthread* thread)
{
    return !thread->is_running && !thread->is_joining && !thread->thread;
}

static void destroy_thread_if_appropriate(zthread* thread)
{
    if (should_destroy_thread(thread)) {
        PAS_ASSERT(!thread->thread);
        PAS_ASSERT(!thread->is_running);
        PAS_ASSERT(!thread->is_joining);
        thread->callback = NULL;
        filc_ptr_store(&thread->arg_ptr, filc_ptr_forge_invalid(NULL));
        filc_ptr_store(&thread->result_ptr, filc_ptr_forge_invalid(NULL));
        filc_ptr_store(&thread->cookie_ptr, filc_ptr_forge_invalid(NULL));
        filc_deallocate_yolo(thread);
    }
}

static void zthread_key_destruct(void* zthread_arg)
{
    zthread* thread;

    thread = (zthread*)zthread_arg;

    sigset_t set;
    PAS_ASSERT(!sigfillset(&set));
    PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &set, NULL));

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(!thread->in_filc);
    PAS_ASSERT(!thread->have_deferred_signals);
    size_t index;
    for (index = MAX_MUSL_SIGNUM + 1; index--;)
        PAS_ASSERT(!thread->num_deferred_signals[index]);
    PAS_ASSERT(thread->is_running);
    thread->is_running = false;

    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock); /* We can do this safely because it's a no-page-sharing isoheap. */
}

static pthread_key_t zthread_key;

struct musl_addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    unsigned ai_addrlen;
    filc_ptr ai_addr;
    filc_ptr ai_canonname;
    filc_ptr ai_next;
};

static const filc_type musl_addrinfo_type = {
    .index = FILC_MUSL_ADDRINFO_TYPE_INDEX,
    .size = sizeof(struct musl_addrinfo),
    .alignment = alignof(struct musl_addrinfo),
    .num_words = sizeof(struct musl_addrinfo) / FILC_WORD_SIZE,
    .u = {
        .trailing_array = NULL
    },
    .word_types = {
        FILC_WORD_TYPE_INT, /* ai_flags, ai_family, ai_socktype, ai_protocol */
        FILC_WORD_TYPE_INT, /* ai_adrlen */
        FILC_WORD_TYPES_PTR, /* ai_addr */
        FILC_WORD_TYPES_PTR, /* ai_canonname */
        FILC_WORD_TYPES_PTR /* ai_next */
    }
};

static pas_heap_ref musl_addrinfo_heap = {
    .type = (const pas_heap_type*)&musl_addrinfo_type,
    .heap = NULL,
    .allocator_index = 0
};

struct musl_dirent {
    uint64_t d_ino;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

typedef struct {
    pas_lock lock;
    DIR* dir;
    uint64_t* musl_to_loc;
    size_t musl_to_loc_size;
    size_t musl_to_loc_capacity;
    pas_uint64_hash_map loc_to_musl;
    struct musl_dirent dirent; /* this gets vended to the user as an int capability. */
} zdirstream;

static void zdirstream_construct(zdirstream* dirstream)
{
    pas_lock_construct(&dirstream->lock);

    /* This indicates that we're not actually constructed, so the hashtables are not in any kind
       of state. */
    dirstream->dir = NULL;
}

DEFINE_RUNTIME_CONFIG(zdirstream_runtime_config, zdirstream, zdirstream_construct);

static const filc_type zdirstream_type = {
    .index = FILC_DIRSTREAM_TYPE_INDEX,
    .size = sizeof(zdirstream),
    .alignment = alignof(zdirstream),
    .num_words = 0,
    .u = {
        .runtime_config = &zdirstream_runtime_config
    },
    .word_types = { }
};

static pas_heap_ref zdirstream_heap = {
    .type = (const pas_heap_type*)&zdirstream_type,
    .heap = NULL,
    .allocator_index = 0
};

/* These are currently leaked. This is fine, because it's super unlikely that someone will call
   sigaction in a loop.
   
   If I find such a fucker, then I'll deal with it then. That would be a fun excuse to do that
   version-multiCAS thing that I came up with that one time. */
typedef struct {
    void (*function)(PIZLONATED_SIGNATURE);
    sigset_t mask;
    int musl_signum; /* This is only needed for assertion discipline. */
} signal_handler;

static signal_handler* signal_table[MAX_MUSL_SIGNUM + 1];

static bool is_initialized = false; /* Useful for assertions. */

static void initialize_impl(void)
{
    filc_type_array = (const filc_type**)filc_allocate_utility(
        sizeof(const filc_type*) * FILC_TYPE_ARRAY_INITIAL_CAPACITY);
    filc_type_array_size = FILC_TYPE_ARRAY_INITIAL_SIZE;
    filc_type_array_capacity = FILC_TYPE_ARRAY_INITIAL_CAPACITY;

    filc_type_array[FILC_INVALID_TYPE_INDEX] = 0;
    filc_type_array[FILC_INT_TYPE_INDEX] = &filc_int_type;
    filc_type_array[FILC_ONE_PTR_TYPE_INDEX] = &filc_one_ptr_type;
    filc_type_array[FILC_INT_PTR_TYPE_INDEX] = &filc_int_ptr_type;
    filc_type_array[FILC_FUNCTION_TYPE_INDEX] = &filc_function_type;
    filc_type_array[FILC_TYPE_TYPE_INDEX] = &filc_type_type;
    filc_type_array[FILC_MUSL_PASSWD_TYPE_INDEX] = &musl_passwd_type;
    filc_type_array[FILC_MUSL_SIGACTION_TYPE_INDEX] = &musl_sigaction_type;
    filc_type_array[FILC_THREAD_TYPE_INDEX] = &zthread_type;
    filc_type_array[FILC_MUSL_ADDRINFO_TYPE_INDEX] = &musl_addrinfo_type;
    filc_type_array[FILC_DIRSTREAM_TYPE_INDEX] = &zdirstream_type;
    
    pthread_key_create(&type_tables_key, type_tables_destroy);
    pthread_key_create(&musl_passwd_threadlocal_key, musl_passwd_threadlocal_destructor);
    
    zthread* result = (zthread*)filc_allocate_opaque(&zthread_heap);
    PAS_ASSERT(!result->thread);
    result->thread = pthread_self();
    result->is_running = true;
    PAS_ASSERT(!should_destroy_thread(result));
    PAS_ASSERT(!pthread_key_create(&zthread_key, zthread_key_destruct));
    PAS_ASSERT(!pthread_setspecific(zthread_key, result));

    is_initialized = true;
}

static pas_system_once global_once = PAS_SYSTEM_ONCE_INIT;

void filc_initialize(void)
{
    pas_system_once_run(&global_once, initialize_impl);
}

static zthread* get_thread(void)
{
    return (zthread*)pthread_getspecific(zthread_key);
}

static void enter_impl(zthread* thread)
{
    PAS_ASSERT(!thread->have_deferred_signals);
    PAS_ASSERT(!thread->in_filc);
    thread->in_filc = true;
}

void filc_enter(void)
{
    enter_impl(get_thread());
}

/* This *just* exits and doesn't do the required post-exit pollcheck! */
static void exit_impl(zthread* thread)
{
    PAS_ASSERT(thread->in_filc);
    thread->in_filc = false;
}

static void call_handler(signal_handler* handler, int signum)
{
    PAS_ASSERT(handler);
    PAS_ASSERT(handler->musl_signum == signum);
    
    uintptr_t return_buffer[2];
    int* args = filc_allocate_int(sizeof(int), 1);
    *args = signum;
    handler->function(args, args + 1, &filc_int_type, return_buffer, return_buffer + 2, &filc_int_type);
}

/* This function works whether we're in filc or not. */
static bool generic_pollcheck(zthread* thread)
{
    static const bool verbose = false;
    
    if (!thread->have_deferred_signals)
        return false;
    thread->have_deferred_signals = false;
    bool in_filc = thread->in_filc;
    if (!in_filc)
        enter_impl(thread);
    size_t index;
    /* I'm guessing at some point I'll actually have to care about the order here? */
    for (index = MAX_MUSL_SIGNUM + 1; index--;) {
        uint64_t num_deferred_signals;
        /* We rely on the CAS for a fence, too. */
        for (;;) {
            num_deferred_signals = thread->num_deferred_signals[index];
            if (pas_compare_and_swap_uint64_weak(
                    thread->num_deferred_signals + index, num_deferred_signals, 0))
                break;
        }
        if (!num_deferred_signals)
            continue;

        if (verbose)
            pas_log("calling signal handler from pollcheck\n");
        
        signal_handler* handler = signal_table[index];
        PAS_ASSERT(handler);
        sigset_t oldset;
        PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &handler->mask, &oldset));
        while (num_deferred_signals--)
            call_handler(handler, (int)index);
        PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
    }
    if (!in_filc)
        exit_impl(thread);
    return true;
}

void filc_exit(void)
{
    zthread* thread = get_thread();
    exit_impl(thread);
    while (generic_pollcheck(thread));
}

void filc_pollcheck(void)
{
    zthread* thread = get_thread();
    PAS_ASSERT(thread->in_filc);
    generic_pollcheck(thread);
}

static void signal_pizlonator(int signum)
{
    static const bool verbose = false;
    
    int musl_signum = to_musl_signum(signum);
    PAS_ASSERT((unsigned)musl_signum <= MAX_MUSL_SIGNUM);
    zthread* thread = get_thread();
    if (thread->in_filc) {
        /* For all we know the user asked for a mask that allows us to recurse, hence the lock-freedom. */
        for (;;) {
            uint64_t old_value = thread->num_deferred_signals[musl_signum];
            if (pas_compare_and_swap_uint64_weak(
                    thread->num_deferred_signals + musl_signum, old_value, old_value + 1))
                break;
        }
        thread->have_deferred_signals = true;
        return;
    }

    filc_enter();
    /* Even if the signal mask allows the signal to recurse, at this point the signal_pizlonator
       will just count and defer. */

    if (verbose)
        pas_log("calling signal handler from pizlonator\n");
    
    call_handler(signal_table[musl_signum], musl_signum);
    filc_exit();
}

void filc_origin_dump(const filc_origin* origin, pas_stream* stream)
{
    if (origin) {
        pas_stream_printf(stream, "%s", origin->filename);
        if (origin->line) {
            pas_stream_printf(stream, ":%u", origin->line);
            if (origin->column)
                pas_stream_printf(stream, ":%u", origin->column);
        }
        if (origin->function)
            pas_stream_printf(stream, ": %s", origin->function);
    } else
        pas_stream_printf(stream, "<somewhere>");
}

static void type_template_dump_impl(const filc_type_template* type, pas_stream* stream)
{
    static const bool dump_ptr = false;

    size_t index;

    if (dump_ptr)
        pas_stream_printf(stream, "@%p", type);

    if (!type) {
        pas_stream_printf(stream, "{null}");
        return;
    }

    if (type == &filc_int_type_template) {
        pas_stream_printf(stream, "{int}");
        return;
    }

    pas_stream_printf(stream, "{%zu,%zu,", type->size, type->alignment);
    for (index = 0; index < filc_type_template_num_words(type); ++index)
        filc_word_type_dump(type->word_types[index], stream);
    if (type->trailing_array) {
        pas_stream_printf(stream, ",trail");
        type_template_dump_impl(type->trailing_array, stream);
    }
    pas_stream_printf(stream, "}");
}

void filc_type_template_dump(const filc_type_template* type, pas_stream* stream)
{
    static const bool dump_only_ptr = false;
    
    if (dump_only_ptr) {
        pas_stream_printf(stream, "%p", type);
        return;
    }

    pas_stream_printf(stream, "type_template");
    type_template_dump_impl(type, stream);
}

static const filc_type* get_type_impl(const filc_type_template* type_template,
                                        pas_lock_hold_mode type_lock_hold_mode);

static const filc_type* get_type_slow_impl(const filc_type_template* type_template)
{
    static const bool verbose = false;
    
    pas_allocation_config allocation_config;
    filc_type_table_add_result add_result;

    PAS_ASSERT(type_template->size);
    PAS_ASSERT(type_template->alignment);
    
    initialize_utility_allocation_config(&allocation_config);

    add_result = filc_type_table_add(
        &filc_type_table_instance, type_template, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        filc_type* result_type;
        size_t index;
        size_t total_size;

        PAS_ASSERT(!pas_mul_uintptr_overflow(
                       sizeof(filc_word_type), filc_type_template_num_words(type_template),
                       &total_size));
        PAS_ASSERT(!pas_add_uintptr_overflow(
                       total_size, PAS_OFFSETOF(filc_type, word_types),
                       &total_size));

        if (filc_type_array_size >= filc_type_array_capacity) {
            const filc_type** new_type_array;
            unsigned new_capacity;
            
            PAS_ASSERT(filc_type_array_size == filc_type_array_capacity);
            PAS_ASSERT(filc_type_array_size <= FILC_TYPE_ARRAY_MAX_SIZE);
            if (filc_type_array_size == FILC_TYPE_ARRAY_MAX_SIZE)
                pas_panic("too many filc types.\n");

            if (filc_type_array_capacity >= FILC_TYPE_ARRAY_MAX_SIZE / 2)
                new_capacity = FILC_TYPE_ARRAY_MAX_SIZE;
            else
                new_capacity = filc_type_array_capacity * 2;
            new_type_array = (const filc_type**)filc_allocate_utility(
                sizeof(filc_type*) * new_capacity);

            memcpy(new_type_array, filc_type_array, filc_type_array_size * sizeof(filc_type*));

            pas_store_store_fence();
            
            filc_type_array = new_type_array;
            filc_type_array_capacity = new_capacity;

            /* Intentionally leak the old array so that there's no race. */
        }

        result_type = filc_allocate_utility(total_size);
        result_type->index = filc_type_array_size++;
        result_type->size = type_template->size;
        result_type->alignment = type_template->alignment;
        result_type->num_words = filc_type_template_num_words(type_template);
        result_type->u.trailing_array = NULL;
        memcpy(result_type->word_types, type_template->word_types,
               filc_type_template_num_words(type_template) * sizeof(filc_word_type));
        
        add_result.entry->type_template = type_template;
        add_result.entry->type = result_type;

        if (type_template->trailing_array) {
            /* Fill in the trailing_array after we've put the hashtable in an OK state (this entry is
               filled in with something). */
            result_type->u.trailing_array = get_type_impl(type_template->trailing_array, pas_lock_is_held);
        }

        filc_validate_type(add_result.entry->type, NULL);

        pas_store_store_fence();

        filc_type_array[result_type->index] = result_type;

        if (verbose)
            pas_log("Created new type %p with index %u\n", result_type, result_type->index);
    }

    return add_result.entry->type;
}

static unsigned fast_type_template_hash(const void* type, void* arg)
{
    PAS_ASSERT(!arg);
    return pas_hash_ptr(type);
}

static const filc_type* get_type_impl(const filc_type_template* type_template,
                                        pas_lock_hold_mode type_lock_hold_mode)
{
    static const bool verbose = false;
    const filc_type* result;
    if (verbose) {
        pas_log("Getting type for ");
        filc_type_template_dump(type_template, &pas_log_stream.base);
        pas_log("\n");
    }
    PAS_ASSERT(type_template);
    if (type_template == &filc_int_type_template)
        return &filc_int_type;
    result = (const filc_type*)pas_lock_free_read_ptr_ptr_hashtable_find(
        &filc_fast_type_table, fast_type_template_hash, NULL, type_template);
    if (result)
        return result;
    filc_type_lock_lock_conditionally(type_lock_hold_mode);
    result = get_type_slow_impl(type_template);
    filc_type_lock_unlock_conditionally(type_lock_hold_mode);
    pas_heap_lock_lock();
    pas_lock_free_read_ptr_ptr_hashtable_set(
        &filc_fast_type_table, fast_type_template_hash, NULL, type_template, result,
        pas_lock_free_read_ptr_ptr_hashtable_set_maybe_existing);
    pas_heap_lock_unlock();
    return result;
}

const filc_type* filc_get_type(const filc_type_template* type_template)
{
    return get_type_impl(type_template, pas_lock_is_not_held);
}

void filc_ptr_dump(filc_ptr ptr, pas_stream* stream)
{
    pas_stream_printf(
        stream, "%p,%p,%p,", filc_ptr_ptr(ptr), filc_ptr_lower(ptr), filc_ptr_upper(ptr));
    filc_type_dump(filc_ptr_type(ptr), stream);
}

static char* ptr_to_new_string_impl(filc_ptr ptr, pas_allocation_config* allocation_config)
{
    pas_string_stream stream;
    pas_string_stream_construct(&stream, allocation_config);
    filc_ptr_dump(ptr, &stream.base);
    return pas_string_stream_take_string(&stream);
}

char* filc_ptr_to_new_string(filc_ptr ptr)
{
    pas_allocation_config allocation_config;
    initialize_utility_allocation_config(&allocation_config);
    return ptr_to_new_string_impl(ptr, &allocation_config);
}

void filc_validate_type(const filc_type* type, const filc_origin* origin)
{
    if (filc_type_is_equal(type, &filc_int_type))
        FILC_ASSERT(type == &filc_int_type, origin);
    if (filc_type_is_equal(type, &filc_function_type))
        FILC_ASSERT(type == &filc_function_type, origin);
    if (filc_type_is_equal(type, &filc_type_type))
        FILC_ASSERT(type == &filc_type_type, origin);
    if (type->size) {
        FILC_ASSERT(type->alignment, origin);
        FILC_ASSERT(pas_is_power_of_2(type->alignment), origin);
        if (!filc_type_get_trailing_array(type))
            FILC_ASSERT(!(type->size % type->alignment), origin);
    } else
        FILC_ASSERT(!type->alignment, origin);
    if (type != &filc_int_type) {
        if (type->num_words) {
            size_t index;
            if (type->u.trailing_array == &filc_int_type) {
                /* You could have an int trailing array starting at an offset that is not a multiple of 16.
                   That's fine so long as the word in which it starts is itself an int. The math inside of
                   filc_type_get_word_type() ends up being wrong in a benign way in that case; for any
                   byte in the trailing array it might return the int word from the tail end of the base
                   type or the int word from the trailing type, and it's arbitrary which you get, and it
                   doesn't matter. These assertions are all about making sure you get int either way,
                   which is right. */
                FILC_ASSERT(pas_round_up_to_power_of_2(type->size, FILC_WORD_SIZE) / FILC_WORD_SIZE
                              == type->num_words, origin);
                FILC_ASSERT(
                    type->size == type->num_words * FILC_WORD_SIZE ||
                    type->word_types[type->num_words - 1] == FILC_WORD_TYPE_INT,
                    origin);
            } else
                FILC_ASSERT(type->size == type->num_words * FILC_WORD_SIZE, origin);
            if (type->u.trailing_array) {
                FILC_ASSERT(type->u.trailing_array->num_words, origin);
                FILC_ASSERT(!type->u.trailing_array->u.trailing_array, origin);
                FILC_ASSERT(type->u.trailing_array->size, origin);
                FILC_ASSERT(type->u.trailing_array->alignment <= type->alignment, origin);
                filc_validate_type(type->u.trailing_array, origin);
            }
            for (index = type->num_words; index--;) {
                FILC_ASSERT(
                    (type->word_types[index] >= FILC_WORD_TYPE_OFF_LIMITS) &&
                    (type->word_types[index] <= FILC_WORD_TYPE_PTR_CAPABILITY),
                    origin);
            }
        }
    }
}

bool filc_type_template_is_equal(const filc_type_template* a, const filc_type_template* b)
{
    size_t index;
    if (a == b)
        return true;
    if (a->size != b->size)
        return false;
    if (a->alignment != b->alignment)
        return false;
    if (a->trailing_array) {
        if (!b->trailing_array)
            return false;
        return filc_type_template_is_equal(a->trailing_array, b->trailing_array);
    } else if (b->trailing_array)
        return false;
    for (index = filc_type_template_num_words(a); index--;) {
        if (a->word_types[index] != b->word_types[index])
            return false;
    }
    return true;
}

unsigned filc_type_template_hash(const filc_type_template* type)
{
    unsigned result;
    size_t index;
    result = type->size + 3 * type->alignment;
    if (type->trailing_array)
        result += 7 * filc_type_template_hash(type->trailing_array);
    for (index = filc_type_template_num_words(type); index--;) {
        result *= 11;
        result += type->word_types[index];
    }
    return result;
}

void filc_word_type_dump(filc_word_type type, pas_stream* stream)
{
    switch (type) {
    case FILC_WORD_TYPE_OFF_LIMITS:
        pas_stream_printf(stream, "/");
        return;
    case FILC_WORD_TYPE_INT:
        pas_stream_printf(stream, "i");
        return;
    case FILC_WORD_TYPE_PTR_SIDECAR:
        pas_stream_printf(stream, "S");
        return;
    case FILC_WORD_TYPE_PTR_CAPABILITY:
        pas_stream_printf(stream, "C");
        return;
    default:
        pas_stream_printf(stream, "?%u", type);
        return;
    }
}

char* filc_word_type_to_new_string(filc_word_type type)
{
    pas_string_stream stream;
    pas_allocation_config allocation_config;
    initialize_utility_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    filc_word_type_dump(type, &stream.base);
    return pas_string_stream_take_string(&stream);
}

static void type_dump_impl(const filc_type* type, pas_stream* stream)
{
    static const bool dump_ptr = false;

    size_t index;

    if (dump_ptr)
        pas_stream_printf(stream, "@%p", type);

    if (!type) {
        pas_stream_printf(stream, "{invalid}");
        return;
    }

    if (type == &filc_function_type) {
        pas_stream_printf(stream, "{function}");
        return;
    }

    if (type == &filc_type_type) {
        pas_stream_printf(stream, "{type}");
        return;
    }

    if (type == &filc_int_type) {
        pas_stream_printf(stream, "{int}");
        return;
    }

    if (!type->num_words) {
        pas_stream_printf(stream, "{unique:%u,%p", type->index, type);
        if (type->size)
            pas_stream_printf(stream, ",%zu,%zu", type->size, type->alignment);
        if (type->u.runtime_config)
            pas_stream_printf(stream, ",%p", type->u.runtime_config);
        pas_stream_printf(stream, "}");
        return;
    }
    
    pas_stream_printf(stream, "{%zu,%zu,", type->size, type->alignment);
    for (index = 0; index < type->num_words; ++index)
        filc_word_type_dump(type->word_types[index], stream);
    if (type->u.trailing_array) {
        pas_stream_printf(stream, ",trail");
        type_dump_impl(type->u.trailing_array, stream);
    }
    pas_stream_printf(stream, "}");
}

void filc_type_dump(const filc_type* type, pas_stream* stream)
{
    static const bool dump_only_ptr = false;
    
    if (dump_only_ptr) {
        pas_stream_printf(stream, "%p", type);
        return;
    }

    pas_stream_printf(stream, "type");
    type_dump_impl(type, stream);
}

pas_heap_runtime_config* filc_type_as_heap_type_get_runtime_config(
    const pas_heap_type* heap_type, pas_heap_runtime_config* config)
{
    const filc_type* type = (const filc_type*)heap_type;
    if (!type->num_words && type->u.runtime_config)
        return &type->u.runtime_config->base;
    return config;
}

pas_heap_runtime_config* filc_type_as_heap_type_assert_default_runtime_config(
    const pas_heap_type* heap_type, pas_heap_runtime_config* config)
{
    const filc_type* type = (const filc_type*)heap_type;
    PAS_ASSERT(!(!type->num_words && type->u.runtime_config));
    return config;
}

void filc_type_as_heap_type_dump(const pas_heap_type* type, pas_stream* stream)
{
    filc_type_dump((const filc_type*)type, stream);
}

static char* type_to_new_string_impl(const filc_type* type, pas_allocation_config* allocation_config)
{
    pas_string_stream stream;
    pas_string_stream_construct(&stream, allocation_config);
    filc_type_dump(type, &stream.base);
    return pas_string_stream_take_string(&stream);
}

char* filc_type_to_new_string(const filc_type* type)
{
    pas_allocation_config allocation_config;
    initialize_utility_allocation_config(&allocation_config);
    return type_to_new_string_impl(type, &allocation_config);
}

static type_tables* get_type_tables(void)
{
    type_tables* result;
    result = pthread_getspecific(type_tables_key);
    if (!result) {
        result = filc_allocate_utility(sizeof(type_tables));
        filc_slice_table_construct(&result->slice_table);
        filc_cat_table_construct(&result->cat_table);
        pthread_setspecific(type_tables_key, result);
    }
    return result;
}

static void check_int_slice_range(const filc_type* type, pas_range range,
                                  const filc_origin* origin)
{
    uintptr_t offset;
    uintptr_t first_word_type_index;
    uintptr_t last_word_type_index;
    uintptr_t word_type_index;

    if (type == &filc_int_type)
        return;

    offset = range.begin;
    first_word_type_index = offset / FILC_WORD_SIZE;
    last_word_type_index = (offset + pas_range_size(range) - 1) / FILC_WORD_SIZE;

    for (word_type_index = first_word_type_index;
         word_type_index <= last_word_type_index;
         word_type_index++) {
        FILC_CHECK(
            filc_type_get_word_type(type, word_type_index) == FILC_WORD_TYPE_INT,
            origin,
            "cannot slice %zu...%zu as int, span contains non-ints (type = %s).",
            range.begin, range.end, filc_type_to_new_string(type));
    }
}

const filc_type* filc_type_slice(const filc_type* type, pas_range range, const filc_origin* origin)
{
    filc_slice_table* table;
    filc_slice_table_key key;
    filc_deep_slice_table_add_result add_result;
    pas_allocation_config allocation_config;
    filc_slice_table_entry new_entry;
    uintptr_t offset_in_type;
    filc_slice_table_entry* entry;
    
    if (type == &filc_int_type)
        return &filc_int_type;

    FILC_CHECK(
        type->num_words,
        origin,
        "cannot slice opaque types like %s.",
        filc_type_to_new_string(type));

    if (type->u.trailing_array && range.begin >= type->size) {
        range = pas_range_sub(range, type->size);
        type = type->u.trailing_array;
        if (type == &filc_int_type)
            return &filc_int_type;
    }
    offset_in_type = range.begin % type->size;
    PAS_ASSERT(offset_in_type == range.begin || !type->u.trailing_array);
    PAS_ASSERT(range.begin >= offset_in_type);
    range = pas_range_sub(range, range.begin - offset_in_type);

    if (!range.begin && range.end == type->size && !type->u.trailing_array)
        return type;
    
    if ((range.begin % FILC_WORD_SIZE) || (range.end % FILC_WORD_SIZE)) {
        check_int_slice_range(type, range, origin);
        return &filc_int_type;
    }

    PAS_TESTING_ASSERT(!(pas_range_size(range) % FILC_WORD_SIZE));

    table = &get_type_tables()->slice_table;
    key.base_type = type;
    key.range = range;
    entry = filc_slice_table_find(table, key);
    if (entry)
        return entry->result_type;

    initialize_utility_allocation_config(&allocation_config);
    
    filc_type_ops_lock_lock();
    add_result = filc_deep_slice_table_add(&filc_global_slice_table, key, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        filc_type_template* type_template;
        size_t index;

        size_t total_size;
        FILC_CHECK(
            !pas_mul_uintptr_overflow(
                sizeof(filc_word_type), pas_range_size(range) / FILC_WORD_SIZE, &total_size),
            origin,
            "range too big (integer overflow).");
        FILC_CHECK(
            !pas_add_uintptr_overflow(
                total_size, PAS_OFFSETOF(filc_type_template, word_types), &total_size),
            origin,
            "range too big (integer overflow).");

        type_template = (filc_type_template*)filc_allocate_utility(total_size);
        type_template->size = pas_range_size(range);
        type_template->alignment = type->alignment;
        while (!pas_is_aligned(range.begin, type_template->alignment) ||
               !pas_is_aligned(range.end, type_template->alignment) ||
               !pas_is_aligned(pas_range_size(range), type_template->alignment))
            type_template->alignment /= 2;
        PAS_ASSERT(type_template->alignment >= FILC_WORD_SIZE);
        type_template->trailing_array = NULL;
        for (index = pas_range_size(range) / FILC_WORD_SIZE; index--;) {
            type_template->word_types[index] =
                filc_type_get_word_type(type, index + range.begin / FILC_WORD_SIZE);
        }
        
        add_result.entry->key = key;
        add_result.entry->result_type = filc_get_type(type_template);
    }
    filc_type_ops_lock_unlock();

    new_entry.key = key;
    new_entry.result_type = add_result.entry->result_type;
    filc_slice_table_add_new(table, new_entry, NULL, &allocation_config);

    return new_entry.result_type;
}

const filc_type* filc_type_cat(const filc_type* a, size_t a_size,
                                   const filc_type* b, size_t b_size,
                                   const filc_origin* origin)
{
    filc_cat_table* table;
    filc_cat_table_key key;
    filc_deep_cat_table_add_result add_result;
    pas_allocation_config allocation_config;
    filc_cat_table_entry new_entry;
    filc_cat_table_entry* entry;

    if (a == &filc_int_type && b == &filc_int_type)
        return &filc_int_type;

    FILC_CHECK(
        a->num_words,
        origin,
        "cannot cat opaque types like %s.",
        filc_type_to_new_string(a));
    FILC_CHECK(
        b->num_words,
        origin,
        "cannot cat opaque types like %s.",
        filc_type_to_new_string(b));
    FILC_CHECK(
        pas_is_aligned(a_size, b->alignment),
        origin,
        "a_size %zu is not aligned to b type %s; refusing to add alignment padding for you.",
        a_size, filc_type_to_new_string(b));

    if ((a_size % FILC_WORD_SIZE)) {
        check_int_slice_range(a, pas_range_create(0, a_size), origin);
        check_int_slice_range(b, pas_range_create(0, b_size), origin);
        return &filc_int_type;
    }

    if ((b_size % FILC_WORD_SIZE)) {
        check_int_slice_range(b, pas_range_create(0, b_size), origin);
        b_size = pas_round_up_to_power_of_2(b_size, FILC_WORD_SIZE);
    }

    table = &get_type_tables()->cat_table;
    key.a = a;
    key.a_size = a_size;
    key.b = b;
    key.b_size = b_size;
    entry = filc_cat_table_find(table, key);
    if (entry)
        return entry->result_type;

    initialize_utility_allocation_config(&allocation_config);

    filc_type_ops_lock_lock();
    add_result = filc_deep_cat_table_add(&filc_global_cat_table, key, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        filc_type_template* result_type;
        size_t index;
        size_t new_type_size;
        size_t total_size;
        size_t alignment;
        size_t aligned_size;

        FILC_CHECK(
            !pas_add_uintptr_overflow(a_size, b_size, &new_type_size),
            origin,
            "sizes too big (integer overflow).");
        FILC_CHECK(
            !pas_mul_uintptr_overflow(
                sizeof(filc_word_type), new_type_size / FILC_WORD_SIZE, &total_size),
            origin,
            "sizes too big (integer overflow).");
        FILC_CHECK(
            !pas_add_uintptr_overflow(
                total_size, PAS_OFFSETOF(filc_type_template, word_types), &total_size),
            origin,
            "sizes too big (integer overflow).");
        alignment = pas_max_uintptr(a->alignment, b->alignment);
        aligned_size = pas_round_up_to_power_of_2(new_type_size, alignment);
        FILC_CHECK(
            aligned_size >= new_type_size,
            origin,
            "sizes too big (hella strange alignment-related integer overflow).");

        result_type = (filc_type_template*)filc_allocate_utility(total_size);

        result_type->size = aligned_size;
        result_type->alignment = alignment;
        result_type->trailing_array = NULL;
        PAS_ASSERT(pas_is_aligned(result_type->size, result_type->alignment));
        for (index = a_size / FILC_WORD_SIZE; index--;)
            result_type->word_types[index] = filc_type_get_word_type(a, index);
        for (index = b_size / FILC_WORD_SIZE; index--;) {
            result_type->word_types[index + a_size / FILC_WORD_SIZE] =
                filc_type_get_word_type(b, index);
        }
        for (index = (aligned_size - new_type_size) / FILC_WORD_SIZE; index--;)
            result_type->word_types[index + new_type_size / FILC_WORD_SIZE] = FILC_WORD_TYPE_OFF_LIMITS;

        add_result.entry->key = key;
        add_result.entry->result_type = filc_get_type(result_type);
    }

    filc_type_ops_lock_unlock();

    new_entry.key = key;
    new_entry.result_type = add_result.entry->result_type;
    filc_cat_table_add_new(table, new_entry, NULL, &allocation_config);

    return new_entry.result_type;
}

static pas_heap_ref* get_heap_slow_impl(filc_heap_table* table, const filc_type* type)
{
    pas_allocation_config allocation_config;
    filc_heap_table_add_result add_result;

    PAS_ASSERT(type != &filc_int_type);
    PAS_ASSERT(type->size);
    PAS_ASSERT(type->alignment);
    filc_validate_type(type, NULL);
    
    initialize_utility_allocation_config(&allocation_config);

    add_result = filc_heap_table_add(table, type, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        add_result.entry->type = type;
        add_result.entry->heap = (pas_heap_ref*)filc_allocate_utility(sizeof(pas_heap_ref));
        add_result.entry->heap->type = (const pas_heap_type*)type;
        add_result.entry->heap->heap = NULL;
        add_result.entry->heap->allocator_index = 0;
    }

    return add_result.entry->heap;
}

static unsigned fast_type_hash(const void* type, void* arg)
{
    PAS_ASSERT(!arg);
    return (uintptr_t)type / sizeof(filc_type);
}

static pas_heap_ref* get_heap_impl(pas_lock_free_read_ptr_ptr_hashtable* fast_table,
                                   filc_heap_table* slow_table,
                                   const filc_type* type)
{
    static const bool verbose = false;
    pas_heap_ref* result;
    if (verbose) {
        pas_log("Getting heap for ");
        filc_type_dump(type, &pas_log_stream.base);
        pas_log("\n");
    }
    /* FIXME: This only needs the lock-free table now. And we should make the lock-free table capable
       of using allocators that don't need the heap_lock. */
    PAS_ASSERT(type);
    result = (pas_heap_ref*)pas_lock_free_read_ptr_ptr_hashtable_find(
        fast_table, fast_type_hash, NULL, type);
    if (result)
        return result;
    filc_type_lock_lock();
    result = get_heap_slow_impl(slow_table, type);
    filc_type_lock_unlock();
    pas_heap_lock_lock();
    pas_lock_free_read_ptr_ptr_hashtable_set(
        fast_table, fast_type_hash, NULL, type, result,
        pas_lock_free_read_ptr_ptr_hashtable_set_maybe_existing);
    pas_heap_lock_unlock();
    return result;
}

pas_heap_ref* filc_get_heap(const filc_type* type)
{
    return get_heap_impl(&filc_fast_heap_table, &filc_normal_heap_table, type);
}

pas_allocator_counts filc_allocator_counts;

pas_intrinsic_heap_support filc_int_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap filc_int_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &filc_int_heap,
        &filc_int_type,
        filc_int_heap_support,
        FILC_HEAP_CONFIG,
        &filc_intrinsic_runtime_config.base);

pas_intrinsic_heap_support filc_utility_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap filc_utility_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &filc_utility_heap,
        &filc_int_type,
        filc_utility_heap_support,
        FILC_HEAP_CONFIG,
        &filc_intrinsic_runtime_config.base);

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    filc_allocate_int_impl,
    FILC_HEAP_CONFIG,
    &filc_intrinsic_runtime_config.base,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error,
    &filc_int_heap,
    &filc_int_heap_support,
    pas_intrinsic_heap_is_designated);

static size_t get_size(size_t size, size_t count)
{
    size_t total_size;
    if (pas_mul_uintptr_overflow(size, count, &total_size)) {
        pas_panic("allocation size calculation failure; size * count overflowed "
                  "(size = %zu, count = %zu).\n", size, count);
    }
    return total_size;
}

void* filc_allocate_int(size_t size, size_t count)
{
    size = get_size(size, count);
    void* result = filc_allocate_int_impl_ptr(size, 1);
    pas_zero_memory(result, size);
    return result;
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    filc_allocate_int_with_alignment_impl,
    FILC_HEAP_CONFIG,
    &filc_intrinsic_runtime_config.base,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error,
    &filc_int_heap,
    &filc_int_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* filc_allocate_int_with_alignment(size_t size, size_t count, size_t alignment)
{
    size = get_size(size, count);
    void* result = filc_allocate_int_with_alignment_impl_ptr(size, alignment);
    pas_zero_memory(result, size);
    return result;
}

PAS_CREATE_TRY_ALLOCATE(
    filc_allocate_one_impl,
    FILC_HEAP_CONFIG,
    &filc_typed_runtime_config.base,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error);

void* filc_allocate_one(pas_heap_ref* ref)
{
    PAS_TESTING_ASSERT(filc_type_is_normal((const filc_type*)ref->type));
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    void* result = filc_allocate_one_impl_ptr(ref);
    filc_low_level_ptr_safe_bzero(result, ((const filc_type*)ref->type)->size);
    return result;
}

void* filc_allocate_opaque(pas_heap_ref* ref)
{
    PAS_TESTING_ASSERT(!((const filc_type*)ref->type)->num_words);
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    PAS_TESTING_ASSERT(!is_initialized || get_thread()->in_filc);
    return filc_allocate_one_impl_ptr(ref);
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    filc_allocate_many_impl,
    FILC_HEAP_CONFIG,
    &filc_typed_runtime_config.base,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error);

void* filc_allocate_many(pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(filc_type_is_normal((const filc_type*)ref->type));
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    if (count == 1)
        return filc_allocate_one(ref);
    void* result = (void*)filc_allocate_many_impl_by_count(ref, count, 1).begin;
    filc_low_level_ptr_safe_bzero(result, ((const filc_type*)ref->type)->size * count);
    return result;
}

void* filc_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment)
{
    PAS_TESTING_ASSERT(filc_type_is_normal((const filc_type*)ref->type));
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    if (count == 1 && alignment <= PAS_MIN_ALIGN)
        return filc_allocate_one(ref);
    void* result = (void*)filc_allocate_many_impl_by_count(ref, count, alignment).begin;
    filc_low_level_ptr_safe_bzero(result, ((const filc_type*)ref->type)->size * count);
    return result;
}

static size_t get_flex_size(size_t base_size, size_t element_size, size_t count)
{
    size_t extra_size;
    size_t total_size;

    if (pas_mul_uintptr_overflow(element_size, count, &extra_size)) {
        pas_panic("flex allocation size calculation failure; element_size * count overflowed "
                  "(element_size = %zu, count = %zu).\n",
                  element_size, count);
    }

    if (pas_add_uintptr_overflow(base_size, extra_size, &total_size)) {
        pas_panic("flex allocation size calculation failure; base_size + extra_size overflowed "
                  "(base_size = %zu, element_size = %zu, count = %zu).\n",
                  base_size, element_size, count);
    }

    return total_size;
}

void* filc_allocate_int_flex(size_t base_size, size_t element_size, size_t count)
{
    size_t total_size = get_flex_size(base_size, element_size, count);
    void* result = filc_allocate_int_impl_ptr(total_size, 1);
    pas_zero_memory(result, total_size);
    return result;
}

void* filc_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                              size_t alignment)
{
    size_t total_size = get_flex_size(base_size, element_size, count);
    void* result = filc_allocate_int_with_alignment_impl_ptr(total_size, alignment);
    pas_zero_memory(result, total_size);
    return result;
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    filc_allocate_flex_impl,
    FILC_HEAP_CONFIG,
    &filc_flex_runtime_config.base,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error);

void* filc_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    PAS_TESTING_ASSERT(filc_type_get_trailing_array((const filc_type*)ref->type));
    total_size = get_flex_size(base_size, element_size, count);
    void* result = (void*)filc_allocate_flex_impl_by_size(ref, total_size, 1).begin;
    filc_low_level_ptr_safe_bzero(result, pas_round_up_to_power_of_2(total_size, PAS_MIN_ALIGN));
    return result;
}

void* filc_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                          size_t count, size_t alignment)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    PAS_TESTING_ASSERT(filc_type_get_trailing_array((const filc_type*)ref->type));
    total_size = get_flex_size(base_size, element_size, count);
    void* result = (void*)filc_allocate_flex_impl_by_size(ref, total_size, alignment).begin;
    filc_low_level_ptr_safe_bzero(result, pas_round_up_to_power_of_2(total_size, PAS_MIN_ALIGN));
    return result;
}

void* filc_allocate_with_type(const filc_type* type, size_t size)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "filc_allocate_with_type",
        .line = 0,
        .column = 0
    };

    if (verbose) {
        pas_log("allocate_with_type: allocating ");
        filc_type_dump(type, &pas_log_stream.base);
        pas_log(" with size %zu.\n", size);
    }

    void* result;
    if (type == &filc_int_type)
        result = filc_allocate_int(size, 1);
    else {
        FILC_CHECK(
            type->num_words,
            &origin,
            "cannot allocate instance of opaque type (type = %s).",
            filc_type_to_new_string(type));
        FILC_CHECK(
            !type->u.trailing_array,
            &origin,
            "cannot reflectively allocate instance of type with trailing array (type = %s).",
            filc_type_to_new_string(type));
        FILC_CHECK(
            !(size % type->size),
            &origin,
            "cannot allocate %zu bytes of type %s (have %zu remainder).",
            size, filc_type_to_new_string(type), size % type->size);
        result = filc_allocate_many(filc_get_heap(type), size / type->size);
    }

    if (verbose) {
        pas_log("allocate_with_type: allocated ");
        filc_type_dump(type, &pas_log_stream.base);
        pas_log(" with size %zu at %p.\n", size, result);
    }

    return result;
}

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    filc_allocate_utility_impl,
    FILC_HEAP_CONFIG,
    &filc_intrinsic_runtime_config.base,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error,
    &filc_utility_heap,
    &filc_utility_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* filc_allocate_utility(size_t size)
{
    PAS_TESTING_ASSERT(!is_initialized || get_thread()->in_filc);
    return filc_allocate_utility_impl_ptr(size, 1);
}

static void int_copy_and_zero_callback(void* new_ptr, void* old_ptr,
                                       size_t copy_size, size_t new_size)
{
    PAS_ASSERT(new_size >= copy_size);
    memcpy(new_ptr, old_ptr, copy_size);
    pas_zero_memory((char*)new_ptr + copy_size, new_size - copy_size);
}

static void typed_copy_and_zero_callback(void* new_ptr, void* old_ptr,
                                         size_t copy_size, size_t new_size)
{
    PAS_ASSERT(pas_is_aligned((uintptr_t)new_ptr, FILC_WORD_SIZE));
    PAS_ASSERT(pas_is_aligned((uintptr_t)old_ptr, FILC_WORD_SIZE));
    PAS_ASSERT(pas_is_aligned(copy_size, FILC_WORD_SIZE));
    PAS_ASSERT(pas_is_aligned(new_size, FILC_WORD_SIZE));
    PAS_ASSERT(new_size >= copy_size);
    filc_low_level_ptr_safe_memcpy(new_ptr, old_ptr, copy_size);
    filc_low_level_ptr_safe_bzero((char*)new_ptr + copy_size, new_size - copy_size);
}

static void* reallocate_int_impl(pas_uint128 sidecar, pas_uint128 capability,
                                 size_t size, size_t count, size_t alignment,
                                 const filc_origin* origin,
                                 void* (*allocate)(size_t size, size_t alignment),
                                 void (*deallocate)(const void* ptr))
{
    filc_ptr old_ptr = filc_ptr_create(sidecar, capability);
    filc_check_deallocate(old_ptr, origin);
    
    size_t old_size = filc_ptr_available(old_ptr);

    filc_check_access_int(old_ptr, old_size, origin);

    size_t new_size = get_size(size, count);
    void* new_ptr = allocate(new_size, alignment);

    size_t copy_size = pas_min_uintptr(old_size, new_size);
    memcpy(new_ptr, filc_ptr_ptr(old_ptr), copy_size);
    pas_zero_memory((char*)new_ptr + copy_size, new_size - copy_size);

    deallocate(filc_ptr_ptr(old_ptr));
    
    return new_ptr;
}

void* filc_reallocate_int_impl(pas_uint128 sidecar, pas_uint128 capability, size_t size, size_t count,
                               const filc_origin* origin)
{
    return reallocate_int_impl(sidecar, capability, size, count, 1, origin,
                               filc_allocate_int_impl_ptr,
                               filc_deallocate);
}

void* filc_reallocate_int_with_alignment_impl(pas_uint128 sidecar, pas_uint128 capability,
                                              size_t size, size_t count, size_t alignment,
                                              const filc_origin* origin)
{
    return reallocate_int_impl(sidecar, capability, size, count, alignment, origin,
                               filc_allocate_int_with_alignment_impl_ptr,
                               filc_deallocate);
}

static void* reallocate_impl(pas_uint128 sidecar, pas_uint128 capability,
                             pas_heap_ref* ref, size_t count,
                             const filc_origin* origin,
                             pas_allocation_result (*allocate)(
                                 pas_heap_ref* ref, size_t count, size_t alignment),
                             void (*deallocate)(const void* ptr))
{
    const filc_type* new_type = (const filc_type*)ref->type;
    PAS_TESTING_ASSERT(filc_type_is_normal(new_type));
    PAS_TESTING_ASSERT(pas_is_aligned(new_type->size, FILC_WORD_SIZE));

    filc_ptr old_ptr = filc_ptr_create(sidecar, capability);
    filc_check_deallocate(old_ptr, origin);

    size_t old_size = filc_ptr_available(old_ptr);

    FILC_CHECK(
        !(old_size % new_type->size),
        origin,
        "Cannot reallocate ptr whose available isn't a multiple of new type size "
        "(ptr = %s, available = %zu, new type = %s).",
        filc_ptr_to_new_string(old_ptr), old_size, filc_type_to_new_string(new_type));
    
    size_t old_count = old_size / new_type->size;
    filc_restrict(old_ptr, old_count, new_type, origin);

    void* new_ptr = (void*)allocate(ref, count, 1).begin;
    size_t new_size = count * new_type->size;

    size_t copy_size = pas_min_uintptr(old_size, new_size);
    filc_low_level_ptr_safe_memcpy(new_ptr, filc_ptr_ptr(old_ptr), copy_size);
    filc_low_level_ptr_safe_bzero((char*)new_ptr + copy_size, new_size - copy_size);

    deallocate(filc_ptr_ptr(old_ptr));

    return new_ptr;
}

void* filc_reallocate_impl(pas_uint128 sidecar, pas_uint128 capability, pas_heap_ref* ref, size_t count,
                           const filc_origin* origin)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc);
    return reallocate_impl(sidecar, capability, ref, count, origin,
                           filc_allocate_many_impl_by_count,
                           filc_deallocate);
}

void filc_deallocate_yolo(const void* ptr)
{
    pas_deallocate((void*)ptr, FILC_HEAP_CONFIG);
}

void filc_deallocate(const void* ptr)
{
    PAS_TESTING_ASSERT(!is_initialized || get_thread()->in_filc);
    filc_deallocate_yolo(ptr);
}

void filc_check_deallocate_impl(pas_uint128 sidecar, pas_uint128 capability, const filc_origin* origin)
{
    filc_ptr ptr = filc_ptr_create(sidecar, capability);
    
    /* These checks aren't necessary to protect the allocator, which will already rule out
       pointers that don't belong to it. But, they are necessary to prevent the following:
       
       - Attempts to free filc_types.
       
       - Attempts to free other opaque types, which may be allocated in our heap, and which
         may or may not take kindly to being freed without first doing other things or
         performing other checks.
       
       - Attempts to free someone else's object by doing pointer arithmetic or dreaming up
         a pointer with an inttoptr cast. */
    FILC_CHECK(
        filc_ptr_ptr(ptr) == filc_ptr_lower(ptr),
        origin,
        "attempt to free a pointer with ptr != lower (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    
    if (!filc_ptr_ptr(ptr))
        return;
    
    FILC_CHECK(
        filc_ptr_type(ptr),
        origin,
        "attempt to free nonnull pointer with invalid type (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    FILC_CHECK(
        filc_ptr_type(ptr)->num_words,
        origin,
        "attempt to free nonnull pointer to opaque type (ptr = %s).",
        filc_ptr_to_new_string(ptr));
}

void filc_deallocate_safe(filc_ptr ptr)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "filc_deallocate_safe",
        .line = 0,
        .column = 0
    };

    filc_check_deallocate(ptr, &origin);

    filc_deallocate(filc_ptr_ptr(ptr));
}

void pizlonated_f_zfree(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zfree",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_deallocate_safe(ptr);
}

void pizlonated_f_zgetallocsize(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zgetallocsize",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(size_t), &origin);
    *(size_t*)filc_ptr_ptr(rets) = pas_get_allocation_size(filc_ptr_ptr(ptr), FILC_HEAP_CONFIG);
}

void pizlonated_f_zcalloc_multiply(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zcalloc_multiply",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    size_t left = filc_ptr_get_next_size_t(&args, &origin);
    size_t right = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(size_t), &origin);
    *(size_t*)filc_ptr_ptr(rets) = get_size(left, right);
}

pas_heap_ref* filc_get_hard_heap(const filc_type* type)
{
    return get_heap_impl(&filc_fast_hard_heap_table, &filc_hard_heap_table, type);
}

pas_intrinsic_heap_support filc_hard_int_heap_support =
    PAS_INTRINSIC_HEAP_SUPPORT_INITIALIZER;

pas_heap filc_hard_int_heap =
    PAS_INTRINSIC_HEAP_INITIALIZER(
        &filc_hard_int_heap,
        &filc_int_type,
        filc_hard_int_heap_support,
        FILC_HARD_HEAP_CONFIG,
        &filc_hard_int_runtime_config);

PAS_CREATE_TRY_ALLOCATE_INTRINSIC(
    filc_hard_allocate_int_impl,
    FILC_HARD_HEAP_CONFIG,
    &filc_hard_int_runtime_config,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error,
    &filc_hard_int_heap,
    &filc_hard_int_heap_support,
    pas_intrinsic_heap_is_not_designated);

void* filc_hard_allocate_int(size_t size, size_t count)
{
    size = get_size(size, count);
    void* result = filc_hard_allocate_int_impl_ptr(size, 1);
    pas_zero_memory(result, size);
    return result;
}

void* filc_hard_allocate_int_with_alignment(size_t size, size_t count, size_t alignment)
{
    size = get_size(size, count);
    void* result = filc_hard_allocate_int_impl_ptr(size, alignment);
    pas_zero_memory(result, size);
    return result;
}

PAS_CREATE_TRY_ALLOCATE(
    filc_hard_allocate_one_impl,
    FILC_HARD_HEAP_CONFIG,
    &filc_hard_typed_runtime_config,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error);

void* filc_hard_allocate_one(pas_heap_ref* ref)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc_hard);
    PAS_TESTING_ASSERT(filc_type_is_normal((const filc_type*)ref->type));
    void* result = filc_hard_allocate_one_impl_ptr(ref);
    filc_low_level_ptr_safe_bzero(result, ((const filc_type*)ref->type)->size);
    return result;
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    filc_hard_allocate_many_impl,
    FILC_HARD_HEAP_CONFIG,
    &filc_hard_typed_runtime_config,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error);

void* filc_hard_allocate_many(pas_heap_ref* ref, size_t count)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc_hard);
    PAS_TESTING_ASSERT(filc_type_is_normal((const filc_type*)ref->type));
    if (count == 1)
        return filc_hard_allocate_one(ref);
    void* result = (void*)filc_hard_allocate_many_impl_by_count(ref, count, 1).begin;
    filc_low_level_ptr_safe_bzero(result, ((const filc_type*)ref->type)->size * count);
    return result;
}

void* filc_hard_allocate_many_with_alignment(pas_heap_ref* ref, size_t count, size_t alignment)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc_hard);
    PAS_TESTING_ASSERT(filc_type_is_normal((const filc_type*)ref->type));
    if (count == 1 && alignment <= PAS_MIN_ALIGN)
        return filc_hard_allocate_one(ref);
    void* result = (void*)filc_hard_allocate_many_impl_by_count(ref, count, alignment).begin;
    filc_low_level_ptr_safe_bzero(result, ((const filc_type*)ref->type)->size * count);
    return result;
}

void* filc_hard_allocate_int_flex(size_t base_size, size_t element_size, size_t count)
{
    size_t total_size = get_flex_size(base_size, element_size, count);
    void* result = filc_hard_allocate_int_impl_ptr(total_size, 1);
    pas_zero_memory(result, total_size);
    return result;
}

void* filc_hard_allocate_int_flex_with_alignment(size_t base_size, size_t element_size, size_t count,
                                                   size_t alignment)
{
    size_t total_size = get_flex_size(base_size, element_size, count);
    void* result = filc_hard_allocate_int_impl_ptr(total_size, alignment);
    pas_zero_memory(result, total_size);
    return result;
}

PAS_CREATE_TRY_ALLOCATE_ARRAY(
    filc_hard_allocate_flex_impl,
    FILC_HARD_HEAP_CONFIG,
    &filc_hard_flex_runtime_config,
    &filc_allocator_counts,
    pas_allocation_result_crash_on_error);

void* filc_hard_allocate_flex(pas_heap_ref* ref, size_t base_size, size_t element_size, size_t count)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc_hard);
    PAS_TESTING_ASSERT(filc_type_get_trailing_array((const filc_type*)ref->type));
    total_size = get_flex_size(base_size, element_size, count);
    void* result = (void*)filc_hard_allocate_flex_impl_by_size(ref, total_size, 1).begin;
    filc_low_level_ptr_safe_bzero(result, pas_round_up_to_power_of_2(total_size, PAS_MIN_ALIGN));
    return result;
}

void* filc_hard_allocate_flex_with_alignment(pas_heap_ref* ref, size_t base_size, size_t element_size,
                                               size_t count, size_t alignment)
{
    size_t total_size;
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc_hard);
    PAS_TESTING_ASSERT(filc_type_get_trailing_array((const filc_type*)ref->type));
    total_size = get_flex_size(base_size, element_size, count);
    void* result = (void*)filc_hard_allocate_flex_impl_by_size(ref, total_size, alignment).begin;
    filc_low_level_ptr_safe_bzero(result, pas_round_up_to_power_of_2(total_size, PAS_MIN_ALIGN));
    return result;
}

void* filc_hard_reallocate_int_impl(pas_uint128 sidecar, pas_uint128 capability, size_t size, size_t count,
                                    const filc_origin* origin)
{
    return reallocate_int_impl(sidecar, capability, size, count, 1, origin,
                               filc_hard_allocate_int_impl_ptr,
                               filc_hard_deallocate);
}

void* filc_hard_reallocate_int_with_alignment_impl(pas_uint128 sidecar, pas_uint128 capability,
                                                   size_t size, size_t count, size_t alignment,
                                                   const filc_origin* origin)
{
    return reallocate_int_impl(sidecar, capability, size, count, alignment, origin,
                               filc_hard_allocate_int_impl_ptr,
                               filc_hard_deallocate);
}

void* filc_hard_reallocate(pas_uint128 sidecar, pas_uint128 capability, pas_heap_ref* ref, size_t count,
                           const filc_origin* origin)
{
    PAS_TESTING_ASSERT(!ref->heap || ref->heap->config_kind == pas_heap_config_kind_filc_hard);
    return reallocate_impl(sidecar, capability, ref, count, origin,
                           filc_hard_allocate_many_impl_by_count,
                           filc_hard_deallocate);
}

void filc_hard_deallocate(const void* const_ptr)
{
    void* ptr = (void*)const_ptr;
    
    if (!ptr)
        return;
    
    size_t size = pas_get_allocation_size(ptr, FILC_HARD_HEAP_CONFIG);
    if (!size) {
        pas_deallocation_did_fail(
            "attempt to hard deallocate object not allocated by hard heap", (uintptr_t)ptr);
    }

    pas_zero_memory(ptr, size);

    pas_deallocate(ptr, FILC_HARD_HEAP_CONFIG);
}

static filc_ptr new_ptr(void* ptr, size_t size, const filc_type* type)
{
    if (!ptr)
        return filc_ptr_forge_invalid(NULL);
    return filc_ptr_forge(ptr, ptr, (char*)ptr + size, type);
}

pas_uint128 filc_new_capability(void* ptr, size_t size, const filc_type* type)
{
    static const bool verbose = false;
    if (verbose) {
        pas_log("Creating capability with ptr = %p, size = %zu, type = %s\n", ptr, size,
                filc_type_to_new_string(type));
    }
    return new_ptr(ptr, size, type).capability;
}

pas_uint128 filc_new_sidecar(void* ptr, size_t size, const filc_type* type)
{
    static const bool verbose = false;
    if (verbose) {
        pas_log("Creating sidecar with ptr = %p, size = %zu, type = %s\n", ptr, size,
                filc_type_to_new_string(type));
    }
    return new_ptr(ptr, size, type).sidecar;
}

void filc_log_allocation(filc_ptr ptr, const filc_origin* origin)
{
    filc_origin_dump(origin, &pas_log_stream.base);
    pas_log(": allocated ");
    filc_ptr_dump(ptr, &pas_log_stream.base);
    pas_log("\n");
}

void filc_log_allocation_impl(pas_uint128 sidecar, pas_uint128 capability, const filc_origin* origin)
{
    filc_log_allocation(filc_ptr_create(sidecar, capability), origin);
}

void filc_check_forge(
    void* ptr, size_t size, size_t count, const filc_type* type, const filc_origin* origin)
{
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(size, count, &total_size),
        origin,
        "bad zunsafe_forge: size * count overflows (size = %zu, count = %zu).",
        size, count);
    FILC_CHECK(
        pas_is_aligned((uintptr_t)ptr, type->alignment),
        origin,
        "bad zunsafe_forge: pointer is not aligned to the type's alignment (ptr = %p, type = %s).",
        ptr, filc_type_to_new_string(type));
    FILC_CHECK(
        !((uintptr_t)ptr & ~PAS_ADDRESS_MASK),
        origin,
        "bad zunsafe_forge: pointer does not fit in capability (ptr = %p).",
        ptr);
    FILC_CHECK(
        !(((uintptr_t)ptr + total_size) & ~PAS_ADDRESS_MASK),
        origin,
        "bad zunsafe_forge: upper bound does not fit in capability "
        "(ptr = %p, total size = %zu, upper = %p).",
        ptr, total_size, (char*)ptr + total_size);

    /* These things should just be true if the compiler did its job. */
    PAS_TESTING_ASSERT(type->num_words);
    PAS_TESTING_ASSERT(!type->u.trailing_array);
    PAS_TESTING_ASSERT(!(total_size % type->size));
}

void pizlonated_f_zhard_free(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zhard_free",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();

    filc_check_deallocate(ptr, &origin);
    
    filc_hard_deallocate(filc_ptr_ptr(ptr));
}

void pizlonated_f_zhard_getallocsize(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zhard_getallocsize",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(size_t), &origin);
    *(size_t*)filc_ptr_ptr(rets) = pas_get_allocation_size(filc_ptr_ptr(ptr), FILC_HARD_HEAP_CONFIG);
}

void pizlonated_f_zgetlower(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zgetlower",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    /* It's totally fine to store filc_ptrs into the rets without any atomic stuff, because the
       rets buffer is guaranteed to only be read by the caller and then immediately discarded. */
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_with_ptr(ptr, filc_ptr_lower(ptr));
}

void pizlonated_f_zgetupper(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zgetupper",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_with_ptr(ptr, filc_ptr_upper(ptr));
}

void pizlonated_f_zgettype(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zgettype",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) =
        filc_ptr_forge_byte((void*)filc_ptr_type(ptr), &filc_type_type);
}

void pizlonated_f_zslicetype(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zslicetype",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr type_ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t begin = filc_ptr_get_next_size_t(&args, &origin);
    size_t end = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_check_access_opaque(type_ptr, &filc_type_type, &origin);
    const filc_type* result = filc_type_slice(
        (const filc_type*)filc_ptr_ptr(type_ptr), pas_range_create(begin, end), &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte((void*)result, &filc_type_type);
}

void filc_validate_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                              const filc_origin* origin)
{
    static const bool verbose = false;

    filc_ptr ptr;
    filc_ptr borked_ptr;
    const filc_type* expected_borked_type;
    
    /* Have to create this manually because filc_ptr_create calls validate! */
    ptr.sidecar = sidecar;
    ptr.capability = capability;
    borked_ptr.sidecar = 0;
    borked_ptr.capability = capability;

    FILC_ASSERT(filc_ptr_capability_type_index(ptr) < filc_type_array_size, origin);
    FILC_ASSERT(filc_ptr_sidecar_type_index(ptr) < filc_type_array_size, origin);

    if (verbose) {
        pas_log("validating ptr: ");
        filc_ptr_dump(ptr, &pas_log_stream.base);
        pas_log("\n");
    }

    if (filc_ptr_capability_kind(ptr) == filc_capability_oob) {
        FILC_ASSERT(!filc_ptr_capability_type_index(ptr), origin);
        FILC_ASSERT(!filc_ptr_capability_has_things(ptr), origin);
    }

    void* ptr_ptr = filc_ptr_ptr(ptr);
    void* lower = filc_ptr_lower(ptr);
    void* borked_lower = filc_ptr_lower(borked_ptr);
    void* upper = filc_ptr_upper(ptr);
    void* borked_upper = filc_ptr_upper(borked_ptr);
    const filc_type* type = filc_ptr_type(ptr);

    if (!lower)
        FILC_ASSERT(!borked_lower, origin);
    if (ptr_ptr < lower || ptr_ptr > upper) {
        FILC_ASSERT(filc_ptr_capability_kind(ptr) == filc_capability_oob
                      || filc_ptr_is_boxed_int(ptr), origin);
        FILC_ASSERT(!borked_lower, origin);
        FILC_ASSERT(!borked_upper, origin);
    } else {
        FILC_ASSERT(filc_ptr_capability_kind(ptr) != filc_capability_oob, origin);
        FILC_ASSERT(borked_lower >= lower, origin);
        FILC_ASSERT(borked_upper <= upper, origin);
    }
    if (filc_ptr_capability_kind(ptr) == filc_capability_flex_base)
        FILC_ASSERT(borked_lower == lower, origin);
    else if (filc_ptr_capability_kind(ptr) != filc_capability_oob)
        FILC_ASSERT(borked_upper == upper, origin);
    
    if (!type) {
        FILC_ASSERT(!lower, origin);
        FILC_ASSERT(!upper, origin);
        return;
    }

    FILC_ASSERT(lower, origin);
    FILC_ASSERT(upper >= lower, origin);
    FILC_ASSERT(upper <= (void*)PAS_MAX_ADDRESS, origin);
    expected_borked_type = type;
    if (type->size) {
        const filc_type* trailing_type;
        FILC_ASSERT(pas_is_aligned((uintptr_t)lower, type->alignment), origin);
        if (type->num_words) {
            trailing_type = type->u.trailing_array;
            if (trailing_type) {
                FILC_ASSERT(pas_is_aligned((uintptr_t)upper, trailing_type->alignment), origin);
                if (lower != upper) {
                    FILC_ASSERT((size_t)((char*)upper - (char*)lower) >= type->size, origin);
                    FILC_ASSERT(
                        !(((char*)upper - (char*)lower - type->size) % trailing_type->size), origin);
                }
                if (ptr_ptr < lower || (size_t)((char*)ptr_ptr - (char*)lower) >= type->size)
                    expected_borked_type = trailing_type;
            } else {
                FILC_ASSERT(pas_is_aligned((uintptr_t)upper, type->alignment), origin);
                FILC_ASSERT(!(((char*)upper - (char*)lower) % type->size), origin);
            }
        }
    }
    if (filc_ptr_capability_kind(ptr) == filc_capability_oob)
        FILC_ASSERT(!filc_ptr_type(borked_ptr), origin);
    else
        FILC_ASSERT(filc_ptr_type(borked_ptr) == expected_borked_type, origin);
    if (!type->num_words && lower != upper) {
        FILC_ASSERT((char*)upper == (char*)lower + 1, origin);
        if (filc_ptr_capability_kind(ptr) != filc_capability_oob)
            FILC_ASSERT((char*)upper == (char*)borked_lower + 1 || upper == borked_lower, origin);
    }

    filc_validate_type(type, origin);
}

filc_ptr filc_ptr_forge(void* ptr, void* lower, void* upper, const filc_type* type)
{
    static const bool verbose = false;
    filc_ptr result;
    if (verbose)
        pas_log("forging: %p,%p,%p,%s\n", ptr, lower, upper, filc_type_to_new_string(type));
    if (!type || ((uintptr_t)ptr & ~PAS_ADDRESS_MASK)) {
        if (!type) {
            PAS_TESTING_ASSERT(!lower);
            PAS_TESTING_ASSERT(!upper);
        }
        result.capability = (pas_uint128)(uintptr_t)ptr;
        result.sidecar = 0;
    } else {
        unsigned capability_type_index;
        unsigned type_index;
        void* sidecar_lower_or_upper;
        void* sidecar_ptr_or_upper;
        void* capability_upper;
        filc_capability_kind capability_kind;
        filc_sidecar_kind sidecar_kind;
        PAS_TESTING_ASSERT(lower);
        PAS_TESTING_ASSERT(upper);
        if (verbose)
            pas_log("type = %p, index = %u\n", type, type->index);
        PAS_TESTING_ASSERT(filc_type_lookup(type->index) == type);
        type_index = type->index;
        if (ptr >= lower && ptr <= upper) {
            if (ptr == lower)
                capability_kind = filc_capability_at_lower;
            else
                capability_kind = filc_capability_in_array;
            sidecar_kind = filc_sidecar_lower;
            capability_type_index = type_index;
            sidecar_lower_or_upper = lower;
            sidecar_ptr_or_upper = ptr;
            capability_upper = upper;
            if (type->num_words && type->u.trailing_array && ptr != lower) {
                if (ptr < lower || (char*)ptr - (char*)lower >= (ptrdiff_t)type->size)
                    capability_type_index = type->u.trailing_array->index;
                else {
                    PAS_TESTING_ASSERT(ptr > lower && (char*)ptr - (char*)lower < (ptrdiff_t)type->size);
                    capability_kind = filc_capability_flex_base;
                    sidecar_kind = filc_sidecar_flex_upper;
                    sidecar_lower_or_upper = upper;
                    capability_upper = (char*)lower + type->size;
                }
            }
        } else {
            capability_kind = filc_capability_oob;
            sidecar_kind = filc_sidecar_oob_capability;
            capability_upper = NULL;
            capability_type_index = 0;
            sidecar_lower_or_upper = lower;
            sidecar_ptr_or_upper = upper;
        }
        result.capability =
            (pas_uint128)((uintptr_t)ptr & PAS_ADDRESS_MASK) |
            ((pas_uint128)((uintptr_t)capability_upper & PAS_ADDRESS_MASK) << (pas_uint128)48) |
            ((pas_uint128)capability_type_index << (pas_uint128)96) |
            ((pas_uint128)capability_kind << (pas_uint128)126);
        result.sidecar =
            (pas_uint128)((uintptr_t)sidecar_ptr_or_upper & PAS_ADDRESS_MASK) |
            ((pas_uint128)((uintptr_t)sidecar_lower_or_upper & PAS_ADDRESS_MASK) << (pas_uint128)48) |
            ((pas_uint128)type_index << (pas_uint128)96) |
            ((pas_uint128)sidecar_kind << (pas_uint128)126);
    }
    PAS_TESTING_ASSERT(filc_ptr_ptr(result) == ptr);
    if (((uintptr_t)ptr & ~PAS_ADDRESS_MASK)) {
        PAS_TESTING_ASSERT(!filc_ptr_lower(result));
        PAS_TESTING_ASSERT(!filc_ptr_upper(result));
        PAS_TESTING_ASSERT(!filc_ptr_type (result));
    } else {
        PAS_TESTING_ASSERT(filc_ptr_lower(result) == lower);
        PAS_TESTING_ASSERT(filc_ptr_upper(result) == upper);
        PAS_TESTING_ASSERT(filc_ptr_type(result) == type);
    }
    filc_testing_validate_ptr(result);
    return result;
}

void* filc_ptr_ptr_impl(pas_uint128 sidecar, pas_uint128 capability)
{
    return filc_ptr_ptr(filc_ptr_create(sidecar, capability));
}

static void check_access_common_maybe_opaque(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin)
{
    if (PAS_ENABLE_TESTING)
        filc_validate_ptr(ptr, origin);

    /* This check is not strictly necessary, but I like the error message. */
    FILC_CHECK(
        filc_ptr_lower(ptr),
        origin,
        "cannot access pointer with null lower (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    
    FILC_CHECK(
        filc_ptr_ptr(ptr) >= filc_ptr_lower(ptr),
        origin,
        "cannot access pointer with ptr < lower (ptr = %s).", 
        filc_ptr_to_new_string(ptr));

    FILC_CHECK(
        filc_ptr_ptr(ptr) < filc_ptr_upper(ptr),
        origin,
        "cannot access pointer with ptr >= upper (ptr = %s).",
        filc_ptr_to_new_string(ptr));

    FILC_CHECK(
        bytes <= (uintptr_t)((char*)filc_ptr_upper(ptr) - (char*)filc_ptr_ptr(ptr)),
        origin,
        "cannot access %zu bytes when upper - ptr = %zu (ptr = %s).",
        bytes, (size_t)((char*)filc_ptr_upper(ptr) - (char*)filc_ptr_ptr(ptr)),
        filc_ptr_to_new_string(ptr));
        
    FILC_CHECK(
        filc_ptr_type(ptr),
        origin,
        "cannot access ptr with invalid type (ptr = %s).",
        filc_ptr_to_new_string(ptr));
}

static void check_access_common(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin)
{
    check_access_common_maybe_opaque(ptr, bytes, origin);
    
    FILC_CHECK(
        filc_ptr_type(ptr)->num_words,
        origin,
        "cannot access %zu bytes, span has opaque type (ptr = %s).",
        bytes, filc_ptr_to_new_string(ptr));
}

void pizlonated_f_zgettypeslice(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zgettypeslice",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t bytes = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    check_access_common(ptr, bytes, &origin);
    uintptr_t offset = filc_ptr_offset(ptr);
    const filc_type* result = filc_type_slice(
        (const filc_type*)filc_ptr_type(ptr), pas_range_create(offset, offset + bytes), &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte((void*)result, &filc_type_type);
}

void pizlonated_f_zcattype(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zcattype",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr a_ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t a_size = filc_ptr_get_next_size_t(&args, &origin);
    filc_ptr b_ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t b_size = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_check_access_opaque(a_ptr, &filc_type_type, &origin);
    filc_check_access_opaque(b_ptr, &filc_type_type, &origin);
    const filc_type* result = filc_type_cat(
        (const filc_type*)filc_ptr_ptr(a_ptr), a_size,
        (const filc_type*)filc_ptr_ptr(b_ptr), b_size, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte((void*)result, &filc_type_type);
}

void pizlonated_f_zalloc_with_type(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zalloc_with_type",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr type_ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t size = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_check_access_opaque(type_ptr, &filc_type_type, &origin);
    const filc_type* type = (const filc_type*)filc_ptr_ptr(type_ptr);
    if (type != &filc_int_type) {
        /* This is never wrong, since type sizes are a multiple of 16. It just gives folks some forgiveness
           when using this API, which makes it a bit more practical to use it with zcattype. */
        size = pas_round_up_to_power_of_2(size, FILC_WORD_SIZE);
    }
    void* result = filc_allocate_with_type(type, size);
    PAS_ASSERT(result);
    *(filc_ptr*)filc_ptr_ptr(rets) =
        filc_ptr_forge(result, result, (char*)result + size, type);
}

void pizlonated_f_ztype_to_new_string(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "ztype_to_new_string",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr type_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_check_access_opaque(type_ptr, &filc_type_type, &origin);
    const filc_type* type = (const filc_type*)filc_ptr_ptr(type_ptr);
    pas_allocation_config allocation_config;
    initialize_int_allocation_config(&allocation_config);
    char* result = type_to_new_string_impl(type, &allocation_config);
    *(filc_ptr*)filc_ptr_ptr(rets) = 
        filc_ptr_forge(result, result, result + strlen(result) + 1, &filc_int_type);
}

void pizlonated_f_zptr_to_new_string(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zptr_to_new_string",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    initialize_int_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    filc_ptr_dump(ptr, &stream.base);
    char* result = pas_string_stream_take_string(&stream);
    *(filc_ptr*)filc_ptr_ptr(rets) = 
        filc_ptr_forge(result, result, result + strlen(result) + 1, &filc_int_type);
}

static void check_int(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin)
{
    uintptr_t offset;
    uintptr_t first_word_type_index;
    uintptr_t last_word_type_index;
    uintptr_t word_type_index;
    const filc_type* type;

    type = filc_ptr_type(ptr);
    if (type == &filc_int_type)
        return;

    offset = filc_ptr_offset(ptr);
    first_word_type_index = offset / FILC_WORD_SIZE;
    last_word_type_index = (offset + bytes - 1) / FILC_WORD_SIZE;

    for (word_type_index = first_word_type_index;
         word_type_index <= last_word_type_index;
         word_type_index++) {
        FILC_CHECK(
            filc_type_get_word_type(type, word_type_index) == FILC_WORD_TYPE_INT,
            origin,
            "cannot access %zu bytes as int, span contains non-ints (ptr = %s).",
            bytes, filc_ptr_to_new_string(ptr));
    }
}

pas_uint128 filc_update_sidecar(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr)
{
    return filc_ptr_with_ptr(filc_ptr_create(sidecar, capability), new_ptr).sidecar;
}

pas_uint128 filc_update_capability(pas_uint128 sidecar, pas_uint128 capability, void* new_ptr)
{
    return filc_ptr_with_ptr(filc_ptr_create(sidecar, capability), new_ptr).capability;
}

void filc_check_access_int_impl(pas_uint128 sidecar, pas_uint128 capability, uintptr_t bytes,
                                const filc_origin* origin)
{
    filc_ptr ptr;
    /* NOTE: the compiler will never generate a zero-byte check, but the runtime may do it. There
       are times when we are passed a primitive vector and a length and we run this check. If the
       length is zero, we want to permit any pointer (even a null or non-int ptr), since we're
       saying we aren't actually going to access it.
       
       If we ever want to optimize this check out, we just need to move it onto the paths where the
       runtime calls into this function, and allow the compiler to emit code that bypasses it. */
    if (!bytes)
        return;
    ptr = filc_ptr_create(sidecar, capability);
    check_access_common(ptr, bytes, origin);
    check_int(ptr, bytes, origin);
}

void filc_check_access_ptr_impl(pas_uint128 sidecar, pas_uint128 capability,
                                  const filc_origin* origin)
{
    filc_ptr ptr;
    uintptr_t offset;
    uintptr_t word_type_index;
    const filc_type* type;

    ptr = filc_ptr_create(sidecar, capability);

    check_access_common(ptr, sizeof(filc_ptr), origin);

    offset = filc_ptr_offset(ptr);
    FILC_CHECK(
        pas_is_aligned(offset, FILC_WORD_SIZE),
        origin,
        "cannot access memory as ptr without 16-byte alignment; in this case ptr %% 16 = %zu (ptr = %s).",
        (size_t)(offset % FILC_WORD_SIZE), filc_ptr_to_new_string(ptr));
    word_type_index = offset / FILC_WORD_SIZE;
    type = filc_ptr_type(ptr);
    FILC_CHECK(
        filc_type_get_word_type(type, word_type_index + 0) == FILC_WORD_TYPE_PTR_SIDECAR,
        origin,
        "memory type error accessing ptr sidecar (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    /* It's interesting that this next check is almost certainly unnecessary, since no type will ever
       have a sidecar followed by anything other than a capability. */
    FILC_CHECK(
        filc_type_get_word_type(type, word_type_index + 1) == FILC_WORD_TYPE_PTR_CAPABILITY,
        origin,
        "memory type error accessing ptr capability (ptr = %s).",
        filc_ptr_to_new_string(ptr));
}

void filc_check_function_call_impl(pas_uint128 sidecar, pas_uint128 capability,
                                     const filc_origin* origin)
{
    filc_ptr ptr;
    ptr = filc_ptr_create(sidecar, capability);
    check_access_common_maybe_opaque(ptr, 1, origin);
    FILC_CHECK(
        filc_ptr_type(ptr) == &filc_function_type,
        origin,
        "attempt to call pointer that is not a function (ptr = %s).",
        filc_ptr_to_new_string(ptr));
}

void filc_check_access_opaque(
    filc_ptr ptr, const filc_type* expected_type, const filc_origin* origin)
{
    PAS_TESTING_ASSERT(!expected_type->num_words);
    check_access_common_maybe_opaque(ptr, 1, origin);
    FILC_CHECK(
        filc_ptr_type(ptr) == expected_type,
        origin,
        "expected ptr to %s during internal opaque access (ptr = %s).",
        filc_type_to_new_string(expected_type),
        filc_ptr_to_new_string(ptr));
}

void filc_low_level_ptr_safe_bzero(void* raw_ptr, size_t bytes)
{
    static const bool verbose = false;
    size_t words;
    pas_uint128* ptr;
    if (verbose)
        pas_log("bytes = %zu\n", bytes);
    ptr = (pas_uint128*)raw_ptr;
    PAS_ASSERT(pas_is_aligned(bytes, FILC_WORD_SIZE));
    words = bytes / FILC_WORD_SIZE;
    while (words--)
        __c11_atomic_store((_Atomic pas_uint128*)ptr++, 0, __ATOMIC_RELAXED);
}

static void ptr_safe_memcpy_up(void* raw_dst, void* raw_src, size_t words)
{
    pas_uint128* dst;
    pas_uint128* src;
    dst = (pas_uint128*)raw_dst;
    src = (pas_uint128*)raw_src;
    while (words--) {
        __c11_atomic_store(
            (_Atomic pas_uint128*)dst++,
            __c11_atomic_load((_Atomic pas_uint128*)src++, __ATOMIC_RELAXED),
            __ATOMIC_RELAXED);
    }
}

static void ptr_safe_memcpy_down(void* raw_dst, void* raw_src, size_t words)
{
    pas_uint128* dst;
    pas_uint128* src;
    dst = (pas_uint128*)raw_dst;
    src = (pas_uint128*)raw_src;
    while (words--) {
        __c11_atomic_store(
            (_Atomic pas_uint128*)dst + words,
            __c11_atomic_load((_Atomic pas_uint128*)src + words, __ATOMIC_RELAXED),
            __ATOMIC_RELAXED);
    }
}

void filc_low_level_ptr_safe_memcpy(void* dst, void* src, size_t bytes)
{
    PAS_ASSERT(pas_is_aligned(bytes, FILC_WORD_SIZE));
    ptr_safe_memcpy_up(dst, src, bytes / FILC_WORD_SIZE);
}

void filc_low_level_ptr_safe_memmove(void* dst, void* src, size_t bytes)
{
    PAS_ASSERT(pas_is_aligned(bytes, FILC_WORD_SIZE));
    if (dst < src)
        ptr_safe_memcpy_up(dst, src, bytes / FILC_WORD_SIZE);
    else
        ptr_safe_memcpy_down(dst, src, bytes / FILC_WORD_SIZE);
}

void filc_memset_impl(pas_uint128 sidecar, pas_uint128 capability,
                        unsigned value, size_t count, const filc_origin* origin)
{
    static const bool verbose = false;
    filc_ptr ptr;
    char* raw_ptr;
    
    if (!count)
        return;

    ptr = filc_ptr_create(sidecar, capability);
    raw_ptr = filc_ptr_ptr(ptr);

    if (verbose)
        pas_log("count = %zu\n", count);
    check_access_common(ptr, count, origin);
    
    if (!value) {
        const filc_type* type;
        type = filc_ptr_type(ptr);
        if (type != &filc_int_type) {
            uintptr_t offset;
            filc_word_type word_type;
            
            offset = filc_ptr_offset(ptr);
            word_type = filc_type_get_word_type(type, offset / FILC_WORD_SIZE);
            if (offset % FILC_WORD_SIZE) {
                FILC_ASSERT(
                    word_type != FILC_WORD_TYPE_PTR_SIDECAR
                    && word_type != FILC_WORD_TYPE_PTR_CAPABILITY,
                    origin);
            } else {
                FILC_ASSERT(
                    word_type != FILC_WORD_TYPE_PTR_CAPABILITY,
                    origin);
            }
            word_type = filc_type_get_word_type(type, (offset + count - 1) / FILC_WORD_SIZE);
            if ((offset + count) % FILC_WORD_SIZE) {
                FILC_ASSERT(
                    word_type != FILC_WORD_TYPE_PTR_SIDECAR
                    && word_type != FILC_WORD_TYPE_PTR_CAPABILITY,
                    origin);
            } else {
                FILC_ASSERT(
                    word_type != FILC_WORD_TYPE_PTR_SIDECAR,
                    origin);
            }
        }

        filc_exit();
        if ((uintptr_t)raw_ptr % FILC_WORD_SIZE) {
            size_t sliver_size;
            char* new_raw_ptr;
            new_raw_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_ptr, FILC_WORD_SIZE);
            sliver_size = new_raw_ptr - raw_ptr;
            if (sliver_size > count) {
                memset(raw_ptr, 0, count);
                filc_enter();
                return;
            }
            memset(raw_ptr, 0, sliver_size);
            count -= sliver_size;
            raw_ptr = new_raw_ptr;
        }
        filc_low_level_ptr_safe_bzero(raw_ptr, pas_round_down_to_power_of_2(count, FILC_WORD_SIZE));
        raw_ptr += pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        count -= pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        memset(raw_ptr, 0, count);
        filc_enter();
        return;
    }

    check_int(ptr, count, origin);
    filc_exit();
    memset(raw_ptr, value, count);
    filc_enter();
}

enum type_overlap_result {
    int_type_overlap,
    possible_ptr_type_overlap
};

typedef enum type_overlap_result type_overlap_result;

static type_overlap_result check_type_overlap(filc_ptr dst, filc_ptr src,
                                              size_t count, const filc_origin* origin)
{
    const filc_type* dst_type;
    const filc_type* src_type;
    uintptr_t dst_offset;
    uintptr_t src_offset;
    filc_word_type word_type;
    uintptr_t num_word_types;
    uintptr_t word_type_index_offset;
    uintptr_t first_dst_word_type_index;
    uintptr_t first_src_word_type_index;
    type_overlap_result result;

    dst_type = filc_ptr_type(dst);
    src_type = filc_ptr_type(src);

    if (dst_type == &filc_int_type && src_type == &filc_int_type)
        return int_type_overlap;

    dst_offset = filc_ptr_offset(dst);
    src_offset = filc_ptr_offset(src);
    if ((dst_offset % FILC_WORD_SIZE) != (src_offset % FILC_WORD_SIZE)) {
        /* No chance we could copy pointers if the offsets are skewed within a word.
           
           It would be harder to write a generalized checking algorithm for this case (the
           non-offset-skew version lower in this function can't handle it) and we don't need
           to, since that path could only succeed if there were only integers on both sides of
           the copy. */
        check_int(dst, count, origin);
        check_int(src, count, origin);
        return int_type_overlap;
    }

    result = int_type_overlap;
    
    num_word_types = (dst_offset + count - 1) / FILC_WORD_SIZE - dst_offset / FILC_WORD_SIZE + 1;
    first_dst_word_type_index = dst_offset / FILC_WORD_SIZE;
    first_src_word_type_index = src_offset / FILC_WORD_SIZE;

    for (word_type_index_offset = num_word_types; word_type_index_offset--;) {
        filc_word_type dst_word_type;
        filc_word_type src_word_type;

        dst_word_type = filc_type_get_word_type(
            dst_type, first_dst_word_type_index + word_type_index_offset);
        src_word_type = filc_type_get_word_type(
            src_type, first_src_word_type_index + word_type_index_offset);
        
        FILC_CHECK(
            dst_word_type == src_word_type,
            origin,
            "memory type overlap error at offset %zu; dst = %s, src = %s, count = %zu. "
            "dst has %s while src has %s.",
            word_type_index_offset * FILC_WORD_SIZE,
            filc_ptr_to_new_string(dst), filc_ptr_to_new_string(src), count,
            filc_word_type_to_new_string(dst_word_type),
            filc_word_type_to_new_string(src_word_type));

        if (dst_word_type != FILC_WORD_TYPE_INT)
            result = possible_ptr_type_overlap;
    }

    /* We cannot copy parts of pointers. */
    word_type = filc_type_get_word_type(dst_type, first_dst_word_type_index);
    if (dst_offset % FILC_WORD_SIZE) {
        FILC_ASSERT(
            word_type != FILC_WORD_TYPE_PTR_SIDECAR
            && word_type != FILC_WORD_TYPE_PTR_CAPABILITY,
            origin);
    } else {
        FILC_ASSERT(
            word_type != FILC_WORD_TYPE_PTR_CAPABILITY,
            origin);
    }
    word_type = filc_type_get_word_type(dst_type, first_dst_word_type_index + num_word_types - 1);
    if ((dst_offset + count) % FILC_WORD_SIZE) {
        FILC_ASSERT(
            word_type != FILC_WORD_TYPE_PTR_SIDECAR
            && word_type != FILC_WORD_TYPE_PTR_CAPABILITY,
            origin);
    } else {
        FILC_ASSERT(
            word_type != FILC_WORD_TYPE_PTR_SIDECAR,
            origin);
    }

    return result;
}

static type_overlap_result check_copy(filc_ptr dst, filc_ptr src,
                                      size_t count, const filc_origin* origin)
{
    check_access_common(dst, count, origin);
    check_access_common(src, count, origin);
    return check_type_overlap(dst, src, count, origin);
}

/* NOTE: There's an argument to be made that memcpy in Deluge should always memmove, because:
   
   - The overhead of Deluge is likely to make the small perf difference between memcpy and memmove
     irrelevant.

   - If you're copying a range that includes any pointers then it's unlikely that there is any
     difference in performance between memcpy and memmove at all. */
void filc_memcpy_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                        pas_uint128 src_sidecar, pas_uint128 src_capability,
                        size_t count, const filc_origin* origin)
{
    filc_ptr dst;
    filc_ptr src;
    char* raw_dst_ptr;
    char* raw_src_ptr;
    
    if (!count)
        return;

    dst = filc_ptr_create(dst_sidecar, dst_capability);
    src = filc_ptr_create(src_sidecar, src_capability);

    raw_dst_ptr = filc_ptr_ptr(dst);
    raw_src_ptr = filc_ptr_ptr(src);
    
    switch (check_copy(dst, src, count, origin)) {
    case int_type_overlap:
        filc_exit();
        memcpy(raw_dst_ptr, raw_src_ptr, count);
        filc_enter();
        return;
    case possible_ptr_type_overlap: {
        filc_exit();
        PAS_TESTING_ASSERT(
            ((uintptr_t)raw_dst_ptr % FILC_WORD_SIZE) == ((uintptr_t)raw_src_ptr % FILC_WORD_SIZE));
        if ((uintptr_t)raw_dst_ptr % FILC_WORD_SIZE) {
            size_t sliver_size;
            char* new_raw_dst_ptr;
            char* new_raw_src_ptr;
            new_raw_dst_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_dst_ptr, FILC_WORD_SIZE);
            new_raw_src_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_src_ptr, FILC_WORD_SIZE);
            sliver_size = new_raw_dst_ptr - raw_dst_ptr;
            if (sliver_size > count) {
                memcpy(raw_dst_ptr, raw_src_ptr, count);
                filc_enter();
                return;
            }
            memcpy(raw_dst_ptr, raw_src_ptr, sliver_size);
            count -= sliver_size;
            raw_dst_ptr = new_raw_dst_ptr;
            raw_src_ptr = new_raw_src_ptr;
        }
        filc_low_level_ptr_safe_memcpy(
            raw_dst_ptr, raw_src_ptr, pas_round_down_to_power_of_2(count, FILC_WORD_SIZE));
        raw_dst_ptr += pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        raw_src_ptr += pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        count -= pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        memcpy(raw_dst_ptr, raw_src_ptr, count);
        filc_enter();
        return;
    } }
    PAS_ASSERT(!"Should not be reached");
}

void filc_memmove_impl(pas_uint128 dst_sidecar, pas_uint128 dst_capability,
                         pas_uint128 src_sidecar, pas_uint128 src_capability,
                         size_t count, const filc_origin* origin)
{
    filc_ptr dst;
    filc_ptr src;
    char* raw_dst_ptr;
    char* raw_src_ptr;
    
    if (!count)
        return;

    dst = filc_ptr_create(dst_sidecar, dst_capability);
    src = filc_ptr_create(src_sidecar, src_capability);

    raw_dst_ptr = filc_ptr_ptr(dst);
    raw_src_ptr = filc_ptr_ptr(src);
    
    switch (check_copy(dst, src, count, origin)) {
    case int_type_overlap:
        filc_exit();
        memmove(raw_dst_ptr, raw_src_ptr, count);
        filc_enter();
        return;
    case possible_ptr_type_overlap: {
        filc_exit();
        void* low_dst;
        void* low_src;
        size_t low_count;
        void* mid_dst;
        void* mid_src;
        size_t mid_count;
        void* high_dst;
        void* high_src;
        size_t high_count;
        size_t sliver_size;
        char* new_raw_dst_ptr;
        char* new_raw_src_ptr;
        PAS_TESTING_ASSERT(
            ((uintptr_t)raw_dst_ptr % FILC_WORD_SIZE) == ((uintptr_t)raw_src_ptr % FILC_WORD_SIZE));
        new_raw_dst_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_dst_ptr, FILC_WORD_SIZE);
        new_raw_src_ptr = (char*)pas_round_up_to_power_of_2((uintptr_t)raw_src_ptr, FILC_WORD_SIZE);
        sliver_size = new_raw_dst_ptr - raw_dst_ptr;
        if (sliver_size > count) {
            memmove(raw_dst_ptr, raw_src_ptr, count);
            filc_enter();
            return;
        }
        low_dst = raw_dst_ptr;
        low_src = raw_src_ptr;
        low_count = sliver_size;
        count -= sliver_size;
        raw_dst_ptr = new_raw_dst_ptr;
        raw_src_ptr = new_raw_src_ptr;
        mid_dst = raw_dst_ptr;
        mid_src = raw_src_ptr;
        mid_count = pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        raw_dst_ptr += pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        raw_src_ptr += pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        count -= pas_round_down_to_power_of_2(count, FILC_WORD_SIZE);
        high_dst = raw_dst_ptr;
        high_src = raw_src_ptr;
        high_count = count;
        if (low_dst < low_src) {
            memmove(low_dst, low_src, low_count);
            filc_low_level_ptr_safe_memmove(mid_dst, mid_src, mid_count);
            memmove(high_dst, high_src, high_count);
        } else {
            memmove(high_dst, high_src, high_count);
            filc_low_level_ptr_safe_memmove(mid_dst, mid_src, mid_count);
            memmove(low_dst, low_src, low_count);
        }
        filc_enter();
        return;
    } }
    PAS_ASSERT(!"Should not be reached");
}

void filc_check_restrict(pas_uint128 sidecar, pas_uint128 capability,
                         void* new_upper, const filc_type* new_type, const filc_origin* origin)
{
    filc_ptr ptr;
    ptr = filc_ptr_create(sidecar, capability);
    if (!filc_ptr_lower(ptr) && !filc_ptr_ptr(ptr) && !new_upper)
        return;
    check_access_common(ptr, (char*)new_upper - (char*)filc_ptr_ptr(ptr), origin);
    FILC_CHECK(new_type, origin, "cannot restrict to NULL type");
    check_type_overlap(filc_ptr_forge(filc_ptr_ptr(ptr), filc_ptr_ptr(ptr), new_upper, new_type),
                       ptr, (char*)new_upper - (char*)filc_ptr_ptr(ptr), origin);
}

const char* filc_check_and_get_new_str(filc_ptr str, const filc_origin* origin)
{
    size_t available;
    size_t length;
    check_access_common(str, 1, origin);
    available = filc_ptr_available(str);
    length = strnlen((char*)filc_ptr_ptr(str), available);
    FILC_ASSERT(length < available, origin);
    FILC_ASSERT(length + 1 <= available, origin);
    check_int(str, length + 1, origin);

    char* result = filc_allocate_utility(length + 1);
    memcpy(result, filc_ptr_ptr(str), length + 1);
    FILC_ASSERT(!result[length], origin);
    return result;
}

const char* filc_check_and_get_new_str_or_null(filc_ptr ptr, const filc_origin* origin)
{
    if (filc_ptr_ptr(ptr))
        return filc_check_and_get_new_str(ptr, origin);
    return NULL;
}

filc_ptr filc_strdup(const char* str)
{
    if (!str)
        return filc_ptr_forge_invalid(NULL);
    size_t size = strlen(str) + 1;
    char* result = (char*)filc_allocate_int(size, 1);
    memcpy(result, str, size);
    return filc_ptr_forge(result, result, result + size, &filc_int_type);
}

void* filc_va_arg_impl(
    pas_uint128 va_list_sidecar, pas_uint128 va_list_capability,
    size_t count, size_t alignment, const filc_type* type, const filc_origin* origin)
{
    filc_ptr va_list;
    filc_ptr* va_list_impl;
    void* result;
    va_list = filc_ptr_create(va_list_sidecar, va_list_capability);
    filc_check_access_ptr(va_list, origin);
    va_list_impl = (filc_ptr*)filc_ptr_ptr(va_list);
    return filc_ptr_ptr(filc_ptr_get_next(va_list_impl, count, alignment, type, origin));
}

filc_global_initialization_context* filc_global_initialization_context_create(
    filc_global_initialization_context* parent)
{
    static const bool verbose = false;
    
    static const size_t initial_capacity = 1;
    
    filc_global_initialization_context* result;

    if (verbose)
        pas_log("creating context with parent = %p\n", parent);
    
    if (parent) {
        parent->ref_count++;
        return parent;
    }

    filc_global_initialization_lock_lock();
    result = (filc_global_initialization_context*)
        filc_allocate_utility(sizeof(filc_global_initialization_context));
    if (verbose)
        pas_log("new context at %p\n", result);
    result->ref_count = 1;
    pas_ptr_hash_set_construct(&result->seen);
    result->entries = (filc_initialization_entry*)
        filc_allocate_utility(sizeof(filc_initialization_entry) * initial_capacity);
    result->num_entries = 0;
    result->entries_capacity = initial_capacity;

    return result;
}

bool filc_global_initialization_context_add(
    filc_global_initialization_context* context, pas_uint128* pizlonated_gptr, pas_uint128 ptr_capability)
{
    static const bool verbose = false;
    
    pas_allocation_config allocation_config;

    filc_global_initialization_lock_assert_held();

    if (verbose)
        pas_log("dealing with pizlonated_gptr = %p\n", pizlonated_gptr);
    
    if (*pizlonated_gptr) {
        /* This case happens if there is a race like this:
           
           Thread #1: runs global fast path for g_foo, but it's NULL, so it starts to create its
                      context, but doesn't get as far as locking the lock.
           Thread #2: runs global fast path for g_foo, it's NULL, so it runs the initializer, including
                      locking/unlocking the initialization lock and all that.
           Thread #1: finally gets the lock and calls this function, and we find that the global is
                      already initialized. */
        if (verbose)
            pas_log("was already initialized\n");
        return false;
    }

    initialize_utility_allocation_config(&allocation_config);

    if (verbose)
        pas_log("capability = %s\n", filc_ptr_to_new_string(filc_ptr_create(0, ptr_capability)));

    if (!pas_ptr_hash_set_set(&context->seen, pizlonated_gptr, NULL, &allocation_config)) {
        if (verbose)
            pas_log("was already seen\n");
        return false;
    }

    if (context->num_entries >= context->entries_capacity) {
        size_t new_capacity;
        filc_initialization_entry* new_entries;
        
        PAS_ASSERT(context->num_entries = context->entries_capacity);

        new_capacity = context->entries_capacity * 2;
        PAS_ASSERT(new_capacity > context->entries_capacity);
        new_entries = (filc_initialization_entry*)
            filc_allocate_utility(sizeof(filc_initialization_entry) * new_capacity);

        memcpy(new_entries, context->entries, sizeof(filc_initialization_entry) * context->num_entries);

        filc_deallocate(context->entries);

        context->entries = new_entries;
        context->entries_capacity = new_capacity;
        PAS_ASSERT(context->num_entries < context->entries_capacity);
    }

    context->entries[context->num_entries].pizlonated_gptr = pizlonated_gptr;
    context->entries[context->num_entries].ptr_capability = ptr_capability;
    context->num_entries++;
    return true;
}

void filc_global_initialization_context_destroy(filc_global_initialization_context* context)
{
    static const bool verbose = false;
    
    size_t index;
    pas_allocation_config allocation_config;
    
    if (--context->ref_count)
        return;

    if (verbose)
        pas_log("destroying/comitting context at %p\n", context);
    
    pas_store_store_fence();

    for (index = context->num_entries; index--;) {
        PAS_ASSERT(!*context->entries[index].pizlonated_gptr);
        __c11_atomic_store((_Atomic pas_uint128*)context->entries[index].pizlonated_gptr,
                           context->entries[index].ptr_capability, __ATOMIC_RELAXED);
    }

    initialize_utility_allocation_config(&allocation_config);

    filc_deallocate(context->entries);
    pas_ptr_hash_set_destruct(&context->seen, &allocation_config);
    filc_deallocate(context);
    filc_global_initialization_lock_unlock();
}

void filc_alloca_stack_push(filc_alloca_stack* stack, void* alloca)
{
    if (stack->size >= stack->capacity) {
        void** new_array;
        size_t new_capacity;
        PAS_ASSERT(stack->size == stack->capacity);

        new_capacity = (stack->capacity + 1) * 2;
        new_array = (void**)filc_allocate_utility(new_capacity * sizeof(void*));

        memcpy(new_array, stack->array, stack->size * sizeof(void*));

        filc_deallocate(stack->array);

        stack->array = new_array;
        stack->capacity = new_capacity;

        PAS_ASSERT(stack->size < stack->capacity);
    }
    PAS_ASSERT(stack->size < stack->capacity);

    stack->array[stack->size++] = alloca;
}

void filc_alloca_stack_restore(filc_alloca_stack* stack, size_t size)
{
    /* We use our own origin because the compiler would have had to mess up real bad for an error to
       happen here. Not sure it's possible for an error to happen here other than via a miscompile. But,
       even in case of miscompile, we'll always do the type-safe thing here. Worst case, we free allocas
       too soon, but then they are freed into the right heap. */
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "filc_alloca_stack_restore",
        .line = 0,
        .column = 0
    };
    FILC_CHECK(
        size <= stack->size,
        &origin,
        "cannot restore alloca stack to %zu, size is %zu.",
        size, stack->size);
    for (size_t index = size; index < stack->size; ++index)
        filc_deallocate(stack->array[index]);
    stack->size = size;
}

void filc_alloca_stack_destroy(filc_alloca_stack* stack)
{
    size_t index;

    PAS_ASSERT(stack->size <= stack->capacity);

    for (index = stack->size; index--;)
        filc_deallocate(stack->array[index]);
    filc_deallocate(stack->array);
}

static filc_ptr get_constant_value(filc_constant_kind kind, void* target,
                                     filc_global_initialization_context* context)
{
    switch (kind) {
    case filc_function_constant:
        return filc_ptr_forge_byte(target, &filc_function_type);
    case filc_global_constant:
        return filc_ptr_create(
            0, ((pas_uint128 (*)(filc_global_initialization_context*))target)(context));
    case filc_expr_constant: {
        filc_constexpr_node* node = (filc_constexpr_node*)target;
        switch (node->opcode) {
        case filc_constexpr_add_ptr_immediate:
            return filc_ptr_with_offset(get_constant_value(node->left_kind, node->left_target, context),
                                          node->right_value);
        }
        PAS_ASSERT(!"Bad constexpr opcode");
    } }
    PAS_ASSERT(!"Bad relocation kind");
    return filc_ptr_create(0, 0);
}

void filc_execute_constant_relocations(
    void* constant, filc_constant_relocation* relocations, size_t num_relocations,
    filc_global_initialization_context* context)
{
    size_t index;
    PAS_ASSERT(context);
    /* Nothing here needs to be atomic, since the constant doesn't become visible to the universe
       until the initialization context is destroyed. */
    for (index = num_relocations; index--;) {
        filc_constant_relocation* relocation;
        filc_ptr* ptr_ptr;
        relocation = relocations + index;
        PAS_ASSERT(pas_is_aligned(relocation->offset, FILC_WORD_SIZE));
        ptr_ptr = (filc_ptr*)((char*)constant + relocation->offset);
        PAS_ASSERT(!ptr_ptr->sidecar);
        PAS_ASSERT(!ptr_ptr->capability);
        PAS_ASSERT(pas_is_aligned((uintptr_t)ptr_ptr, FILC_WORD_SIZE));
        *ptr_ptr = get_constant_value(relocation->kind, relocation->target, context);
    }
}

static bool did_run_deferred_global_ctors = false;
static void (**deferred_global_ctors)(PIZLONATED_SIGNATURE) = NULL; 
static size_t num_deferred_global_ctors = 0;
static size_t deferred_global_ctors_capacity = 0;

void filc_defer_or_run_global_ctor(void (*global_ctor)(PIZLONATED_SIGNATURE))
{
    if (did_run_deferred_global_ctors) {
        uintptr_t return_buffer[2];
        global_ctor(NULL, NULL, NULL, return_buffer, return_buffer + 2, &filc_int_type);
        return;
    }

    if (num_deferred_global_ctors >= deferred_global_ctors_capacity) {
        void (**new_deferred_global_ctors)(PIZLONATED_SIGNATURE);
        size_t new_deferred_global_ctors_capacity;

        PAS_ASSERT(num_deferred_global_ctors == deferred_global_ctors_capacity);

        new_deferred_global_ctors_capacity = (deferred_global_ctors_capacity + 1) * 2;
        new_deferred_global_ctors = (void (**)(PIZLONATED_SIGNATURE))filc_allocate_utility(
            new_deferred_global_ctors_capacity * sizeof(void (*)(PIZLONATED_SIGNATURE)));

        memcpy(new_deferred_global_ctors, deferred_global_ctors,
               num_deferred_global_ctors * sizeof(void (*)(PIZLONATED_SIGNATURE)));

        filc_deallocate(deferred_global_ctors);

        deferred_global_ctors = new_deferred_global_ctors;
        deferred_global_ctors_capacity = new_deferred_global_ctors_capacity;
    }

    deferred_global_ctors[num_deferred_global_ctors++] = global_ctor;
}

void filc_run_deferred_global_ctors(void)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "filc_run_deferred_global_ctors",
        .line = 0,
        .column = 0
    };
    uintptr_t return_buffer[2];
    FILC_CHECK(
        !did_run_deferred_global_ctors,
        &origin,
        "cannot run deferred global constructors twice.");
    did_run_deferred_global_ctors = true;
    /* It's important to run the destructors in exactly the order in which they were deferred, since
       this allows us to match the priority semantics despite not having the priority. */
    for (size_t index = 0; index < num_deferred_global_ctors; ++index)
        deferred_global_ctors[index](NULL, NULL, NULL, return_buffer, return_buffer + 2, &filc_int_type);
    filc_deallocate(deferred_global_ctors);
    num_deferred_global_ctors = 0;
    deferred_global_ctors_capacity = 0;
}

void pizlonated_f_zrun_deferred_global_ctors(PIZLONATED_SIGNATURE)
{
    PIZLONATED_DELETE_ARGS();
    filc_run_deferred_global_ctors();
}

void filc_panic(const filc_origin* origin, const char* format, ...)
{
    va_list args;
    filc_origin_dump(origin, &pas_log_stream.base);
    pas_log(": ");
    va_start(args, format);
    pas_vlog(format, args);
    va_end(args);
    pas_log("\n");
    pas_panic("thwarted a futile attempt to violate memory safety.\n");
}

void filc_error(const char* reason, const filc_origin* origin)
{
    filc_panic(origin, "filc_error: %s", reason);
}

static void print_str(const char* str)
{
    size_t length;
    length = strlen(str);
    while (length) {
        ssize_t result = write(2, str, length);
        PAS_ASSERT(result);
        if (result < 0 && errno == EINTR)
            continue;
        PAS_ASSERT(result > 0);
        PAS_ASSERT((size_t)result <= length);
        str += result;
        length -= result;
    }
}

void pizlonated_f_zprint(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zprint",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr str_ptr;
    str_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    const char* str = filc_check_and_get_new_str(str_ptr, &origin);
    print_str(str);
    filc_deallocate(str);
}

void pizlonated_f_zprint_long(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zprint_long",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    long x;
    char buf[100];
    x = filc_ptr_get_next_long(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    snprintf(buf, sizeof(buf), "%ld", x);
    print_str(buf);
}

void pizlonated_f_zprint_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zprint_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr ptr;
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    initialize_utility_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    filc_ptr_dump(ptr, &stream.base);
    print_str(pas_string_stream_get_string(&stream));
    pas_string_stream_destruct(&stream);
}

void pizlonated_f_zerror(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zerror",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    const char* str = filc_check_and_get_new_str(filc_ptr_get_next_ptr(&args, &origin), &origin);
    PIZLONATED_DELETE_ARGS();
    pas_panic("zerror: %s\n", str);
}

void pizlonated_f_zstrlen(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zstrlen",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    const char* str = filc_check_and_get_new_str(filc_ptr_get_next_ptr(&args, &origin), &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    *(int*)filc_ptr_ptr(rets) = strlen(str);
    filc_deallocate(str);
}

void pizlonated_f_zisdigit(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zisdigit",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int chr = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    *(int*)filc_ptr_ptr(rets) = isdigit(chr);
}

void pizlonated_f_zfence(PIZLONATED_SIGNATURE)
{
    filc_ptr args = PIZLONATED_ARGS;
    PIZLONATED_DELETE_ARGS();
    pas_fence();
}

void pizlonated_f_zstore_store_fence(PIZLONATED_SIGNATURE)
{
    filc_ptr args = PIZLONATED_ARGS;
    PIZLONATED_DELETE_ARGS();
    pas_store_store_fence();
}

void pizlonated_f_zcompiler_fence(PIZLONATED_SIGNATURE)
{
    filc_ptr args = PIZLONATED_ARGS;
    PIZLONATED_DELETE_ARGS();
    pas_compiler_fence(); /* lmao we don't need this */
}

void pizlonated_f_zunfenced_weak_cas_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zunfenced_weak_cas_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr expected = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr new_value = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(ptr, &origin);
    filc_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)filc_ptr_ptr(rets) = filc_ptr_unfenced_weak_cas(filc_ptr_ptr(ptr), expected, new_value);
}

void pizlonated_f_zweak_cas_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zweak_cas_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr expected = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr new_value = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(ptr, &origin);
    filc_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)filc_ptr_ptr(rets) = filc_ptr_weak_cas(filc_ptr_ptr(ptr), expected, new_value);
}

void pizlonated_f_zunfenced_strong_cas_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zunfenced_strong_cas_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr expected = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr new_value = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(ptr, &origin);
    filc_check_access_ptr(rets, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) =
        filc_ptr_unfenced_strong_cas(filc_ptr_ptr(ptr), expected, new_value);
}

void pizlonated_f_zstrong_cas_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zstrong_cas_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr expected = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr new_value = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(ptr, &origin);
    filc_check_access_ptr(rets, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_strong_cas(filc_ptr_ptr(ptr), expected, new_value);
}

void pizlonated_f_zunfenced_xchg_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zunfenced_xchg_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr new_value = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(ptr, &origin);
    filc_check_access_ptr(rets, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_unfenced_xchg(filc_ptr_ptr(ptr), new_value);
}

void pizlonated_f_zxchg_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zxchg_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr new_value = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(ptr, &origin);
    filc_check_access_ptr(rets, &origin);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_xchg(filc_ptr_ptr(ptr), new_value);
}

void pizlonated_f_zis_runtime_testing_enabled(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zis_runtime_testing_enabled",
        .line = 0,
        .column = 0
    };
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(bool), &origin);
    *(bool*)filc_ptr_ptr(rets) = !!PAS_ENABLE_TESTING;
}

void pizlonated_f_zborkedptr(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zborkedptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr sidecar_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr capability_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_ptr result = filc_ptr_create(sidecar_ptr.sidecar, capability_ptr.capability);
    if (verbose)
        pas_log("sidecar = %s, capability = %s, result = %s\n", filc_ptr_to_new_string(sidecar_ptr), filc_ptr_to_new_string(capability_ptr), filc_ptr_to_new_string(result));
    *(filc_ptr*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zvalidate_ptr(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zvalidate_ptr",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_validate_ptr(ptr, &origin);
}

void pizlonated_f_zscavenger_suspend(PIZLONATED_SIGNATURE)
{
    PIZLONATED_DELETE_ARGS();
    pas_scavenger_suspend();
}

void pizlonated_f_zscavenger_resume(PIZLONATED_SIGNATURE)
{
    PIZLONATED_DELETE_ARGS();
    pas_scavenger_resume();
}

static void (*pizlonated_errno_handler)(PIZLONATED_SIGNATURE);

void pizlonated_f_zregister_sys_errno_handler(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zregister_sys_errno_handler",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr errno_handler = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    FILC_CHECK(
        !pizlonated_errno_handler,
        &origin,
        "errno handler already registered.");
    filc_check_function_call(errno_handler, &origin);
    pizlonated_errno_handler = (void(*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(errno_handler);
}

static void set_musl_errno(int errno_value)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "set_musl_errno",
        .line = 0,
        .column = 0
    };
    uintptr_t return_buffer[2];
    int* args;
    FILC_CHECK(
        pizlonated_errno_handler,
        &origin,
        "errno handler not registered when trying to set errno = %d.", errno_value);
    args = (int*)filc_allocate_int(sizeof(int), 1);
    *args = errno_value;
    memset(return_buffer, 0, sizeof(return_buffer));
    pizlonated_errno_handler(args, args + 1, &filc_int_type,
                          return_buffer, return_buffer + 2, &filc_int_type);
}

static int to_musl_errno(int errno_value)
{
    // FIXME: This must be in sync with musl's bits/errno.h, which feels wrong.
    // FIXME: Wouldn't it be infinitely better if we just gave pizlonated code the real errno?
    switch (errno_value) {
    case EPERM          : return  1;
    case ENOENT         : return  2;
    case ESRCH          : return  3;
    case EINTR          : return  4;
    case EIO            : return  5;
    case ENXIO          : return  6;
    case E2BIG          : return  7;
    case ENOEXEC        : return  8;
    case EBADF          : return  9;
    case ECHILD         : return 10;
    case EAGAIN         : return 11;
    case ENOMEM         : return 12;
    case EACCES         : return 13;
    case EFAULT         : return 14;
    case ENOTBLK        : return 15;
    case EBUSY          : return 16;
    case EEXIST         : return 17;
    case EXDEV          : return 18;
    case ENODEV         : return 19;
    case ENOTDIR        : return 20;
    case EISDIR         : return 21;
    case EINVAL         : return 22;
    case ENFILE         : return 23;
    case EMFILE         : return 24;
    case ENOTTY         : return 25;
    case ETXTBSY        : return 26;
    case EFBIG          : return 27;
    case ENOSPC         : return 28;
    case ESPIPE         : return 29;
    case EROFS          : return 30;
    case EMLINK         : return 31;
    case EPIPE          : return 32;
    case EDOM           : return 33;
    case ERANGE         : return 34;
    case EDEADLK        : return 35;
    case ENAMETOOLONG   : return 36;
    case ENOLCK         : return 37;
    case ENOSYS         : return 38;
    case ENOTEMPTY      : return 39;
    case ELOOP          : return 40;
    case ENOMSG         : return 42;
    case EIDRM          : return 43;
    case ENOSTR         : return 60;
    case ENODATA        : return 61;
    case ETIME          : return 62;
    case ENOSR          : return 63;
    case EREMOTE        : return 66;
    case ENOLINK        : return 67;
    case EPROTO         : return 71;
    case EMULTIHOP      : return 72;
    case EBADMSG        : return 74;
    case EOVERFLOW      : return 75;
    case EILSEQ         : return 84;
    case EUSERS         : return 87;
    case ENOTSOCK       : return 88;
    case EDESTADDRREQ   : return 89;
    case EMSGSIZE       : return 90;
    case EPROTOTYPE     : return 91;
    case ENOPROTOOPT    : return 92;
    case EPROTONOSUPPORT: return 93;
    case ESOCKTNOSUPPORT: return 94;
    case EOPNOTSUPP     : return 95;
    case ENOTSUP        : return 95;
    case EPFNOSUPPORT   : return 96;
    case EAFNOSUPPORT   : return 97;
    case EADDRINUSE     : return 98;
    case EADDRNOTAVAIL  : return 99;
    case ENETDOWN       : return 100;
    case ENETUNREACH    : return 101;
    case ENETRESET      : return 102;
    case ECONNABORTED   : return 103;
    case ECONNRESET     : return 104;
    case ENOBUFS        : return 105;
    case EISCONN        : return 106;
    case ENOTCONN       : return 107;
    case ESHUTDOWN      : return 108;
    case ETOOMANYREFS   : return 109;
    case ETIMEDOUT      : return 110;
    case ECONNREFUSED   : return 111;
    case EHOSTDOWN      : return 112;
    case EHOSTUNREACH   : return 113;
    case EALREADY       : return 114;
    case EINPROGRESS    : return 115;
    case ESTALE         : return 116;
    case EDQUOT         : return 122;
    case ECANCELED      : return 125;
    case EOWNERDEAD     : return 130;
    case ENOTRECOVERABLE: return 131;
    default:
        // FIXME: Eventually, we'll probably have to map weird host errnos. Or, just get rid of this
        // madness and have musl use system errno's.
        PAS_ASSERT(!"Bad errno value");
        return 0;
    }
}

static PAS_NEVER_INLINE void set_errno(int errno_value)
{
    set_musl_errno(to_musl_errno(errno_value));
}

#define check_and_clear(passed_flags_ptr, passed_expected) ({ \
        typeof(passed_flags_ptr) flags_ptr = (passed_flags_ptr); \
        typeof(passed_expected) expected = (passed_expected); \
        bool result; \
        if ((*flags_ptr & expected) == expected) { \
            *flags_ptr &= ~expected; \
            result = true; \
        } else \
            result = false; \
        result; \
    })

struct musl_termios {
    unsigned c_iflag;
    unsigned c_oflag;
    unsigned c_cflag;
    unsigned c_lflag;
    unsigned char c_cc[32];
    unsigned c_ispeed;
    unsigned c_ospeed;
};

static unsigned to_musl_ciflag(unsigned iflag)
{
    unsigned result = 0;
    if (check_and_clear(&iflag, IGNBRK))
        result |= 01;
    if (check_and_clear(&iflag, BRKINT))
        result |= 02;
    if (check_and_clear(&iflag, IGNPAR))
        result |= 04;
    if (check_and_clear(&iflag, PARMRK))
        result |= 010;
    if (check_and_clear(&iflag, INPCK))
        result |= 020;
    if (check_and_clear(&iflag, ISTRIP))
        result |= 040;
    if (check_and_clear(&iflag, INLCR))
        result |= 0100;
    if (check_and_clear(&iflag, IGNCR))
        result |= 0200;
    if (check_and_clear(&iflag, ICRNL))
        result |= 0400;
    if (check_and_clear(&iflag, IXON))
        result |= 02000;
    if (check_and_clear(&iflag, IXANY))
        result |= 04000;
    if (check_and_clear(&iflag, IXOFF))
        result |= 010000;
    if (check_and_clear(&iflag, IMAXBEL))
        result |= 020000;
    if (check_and_clear(&iflag, IUTF8))
        result |= 040000;
    PAS_ASSERT(!iflag);
    return result;
}

static bool from_musl_ciflag(unsigned musl_iflag, tcflag_t* result)
{
    *result = 0;
    if (check_and_clear(&musl_iflag, 01))
        *result |= IGNBRK;
    if (check_and_clear(&musl_iflag, 02))
        *result |= BRKINT;
    if (check_and_clear(&musl_iflag, 04))
        *result |= IGNPAR;
    if (check_and_clear(&musl_iflag, 010))
        *result |= PARMRK;
    if (check_and_clear(&musl_iflag, 020))
        *result |= INPCK;
    if (check_and_clear(&musl_iflag, 040))
        *result |= ISTRIP;
    if (check_and_clear(&musl_iflag, 0100))
        *result |= INLCR;
    if (check_and_clear(&musl_iflag, 0200))
        *result |= IGNCR;
    if (check_and_clear(&musl_iflag, 0400))
        *result |= ICRNL;
    if (check_and_clear(&musl_iflag, 02000))
        *result |= IXON;
    if (check_and_clear(&musl_iflag, 04000))
        *result |= IXANY;
    if (check_and_clear(&musl_iflag, 010000))
        *result |= IXOFF;
    if (check_and_clear(&musl_iflag, 020000))
        *result |= IMAXBEL;
    if (check_and_clear(&musl_iflag, 040000))
        *result |= IUTF8;
    return !musl_iflag;
}

static unsigned to_musl_coflag(unsigned oflag)
{
    unsigned result = 0;
    if (check_and_clear(&oflag, OPOST))
        result |= 01;
    if (check_and_clear(&oflag, ONLCR))
        result |= 04;
    /* Maybe I should teach musl about OXTABS or pass it through? meh? */
    PAS_ASSERT(!(oflag & ~OXTABS));
    return result;
}

static bool from_musl_coflag(unsigned musl_oflag, tcflag_t* result)
{
    *result = 0;
    if (check_and_clear(&musl_oflag, 01))
        *result |= OPOST;
    if (check_and_clear(&musl_oflag, 04))
        *result |= ONLCR;
    return !musl_oflag;
}

static unsigned to_musl_ccflag(unsigned cflag)
{
    unsigned result = 0;
    switch (cflag & CSIZE) {
    case CS5:
        break;
    case CS6:
        result |= 020;
        break;
    case CS7:
        result |= 040;
        break;
    case CS8:
        result |= 060;
        break;
    default:
        PAS_ASSERT(!"Should not be reached");
    }
    cflag &= ~CSIZE;
    if (check_and_clear(&cflag, CSTOPB))
        result |= 0100;
    if (check_and_clear(&cflag, CREAD))
        result |= 0200;
    if (check_and_clear(&cflag, PARENB))
        result |= 0400;
    if (check_and_clear(&cflag, PARODD))
        result |= 01000;
    if (check_and_clear(&cflag, HUPCL))
        result |= 02000;
    if (check_and_clear(&cflag, CLOCAL))
        result |= 04000;
    PAS_ASSERT(!cflag);
    return result;
}

static bool from_musl_ccflag(unsigned musl_cflag, tcflag_t* result)
{
    *result = 0;
    switch (musl_cflag & 060) {
    case 0:
        *result |= CS5;
        break;
    case 020:
        *result |= CS6;
        break;
    case 040:
        *result |= CS7;
        break;
    case 060:
        *result |= CS8;
        break;
    default:
        PAS_ASSERT(!"Should not be reached");
    }
    musl_cflag &= ~060;
    if (check_and_clear(&musl_cflag, 0100))
        *result |= CSTOPB;
    if (check_and_clear(&musl_cflag, 0200))
        *result |= CREAD;
    if (check_and_clear(&musl_cflag, 0400))
        *result |= PARENB;
    if (check_and_clear(&musl_cflag, 01000))
        *result |= PARODD;
    if (check_and_clear(&musl_cflag, 02000))
        *result |= HUPCL;
    if (check_and_clear(&musl_cflag, 04000))
        *result |= CLOCAL;
    return !musl_cflag;
}

static unsigned to_musl_clflag(unsigned lflag)
{
    unsigned result = 0;
    if (check_and_clear(&lflag, ISIG))
        result |= 01;
    if (check_and_clear(&lflag, ICANON))
        result |= 02;
    if (check_and_clear(&lflag, ECHO))
        result |= 010;
    if (check_and_clear(&lflag, ECHOE))
        result |= 020;
    if (check_and_clear(&lflag, ECHOK))
        result |= 040;
    if (check_and_clear(&lflag, ECHONL))
        result |= 0100;
    if (check_and_clear(&lflag, NOFLSH))
        result |= 0200;
    if (check_and_clear(&lflag, TOSTOP))
        result |= 0400;
    if (check_and_clear(&lflag, ECHOCTL))
        result |= 01000;
    if (check_and_clear(&lflag, ECHOPRT))
        result |= 02000;
    if (check_and_clear(&lflag, ECHOKE))
        result |= 04000;
    if (check_and_clear(&lflag, FLUSHO))
        result |= 010000;
    if (check_and_clear(&lflag, PENDIN))
        result |= 040000;
    if (check_and_clear(&lflag, IEXTEN))
        result |= 0100000;
    if (check_and_clear(&lflag, EXTPROC))
        result |= 0200000;
    PAS_ASSERT(!lflag);
    return result;
}

static bool from_musl_clflag(unsigned musl_lflag, tcflag_t* result)
{
    *result = 0;
    if (check_and_clear(&musl_lflag, 01))
        *result |= ISIG;
    if (check_and_clear(&musl_lflag, 02))
        *result |= ICANON;
    if (check_and_clear(&musl_lflag, 010))
        *result |= ECHO;
    if (check_and_clear(&musl_lflag, 020))
        *result |= ECHOE;
    if (check_and_clear(&musl_lflag, 040))
        *result |= ECHOK;
    if (check_and_clear(&musl_lflag, 0100))
        *result |= ECHONL;
    if (check_and_clear(&musl_lflag, 0200))
        *result |= NOFLSH;
    if (check_and_clear(&musl_lflag, 0400))
        *result |= TOSTOP;
    if (check_and_clear(&musl_lflag, 01000))
        *result |= ECHOCTL;
    if (check_and_clear(&musl_lflag, 02000))
        *result |= ECHOPRT;
    if (check_and_clear(&musl_lflag, 04000))
        *result |= ECHOKE;
    if (check_and_clear(&musl_lflag, 010000))
        *result |= FLUSHO;
    if (check_and_clear(&musl_lflag, 040000))
        *result |= PENDIN;
    if (check_and_clear(&musl_lflag, 0100000))
        *result |= IEXTEN;
    if (check_and_clear(&musl_lflag, 0200000))
        *result |= EXTPROC;
    return !musl_lflag;
}

/* musl_cc is a 32-byte array. */
static void to_musl_ccc(cc_t* cc, unsigned char* musl_cc)
{
    musl_cc[0] = cc[VINTR];
    musl_cc[1] = cc[VQUIT];
    musl_cc[2] = cc[VERASE];
    musl_cc[3] = cc[VKILL];
    musl_cc[4] = cc[VEOF];
    musl_cc[5] = cc[VTIME];
    musl_cc[6] = cc[VMIN];
    musl_cc[7] = 0; /* VSWTC */
    musl_cc[8] = cc[VSTART];
    musl_cc[9] = cc[VSTOP];
    musl_cc[10] = cc[VSUSP];
    musl_cc[11] = cc[VEOL];
    musl_cc[12] = cc[VREPRINT];
    musl_cc[13] = cc[VDISCARD];
    musl_cc[14] = cc[VWERASE];
    musl_cc[15] = cc[VLNEXT];
    musl_cc[16] = cc[VEOL2];
}

static void from_musl_ccc(unsigned char* musl_cc, cc_t* cc)
{
    cc[VINTR] = musl_cc[0];
    cc[VQUIT] = musl_cc[1];
    cc[VERASE] = musl_cc[2];
    cc[VKILL] = musl_cc[3];
    cc[VEOF] = musl_cc[4];
    cc[VTIME] = musl_cc[5];
    cc[VMIN] = musl_cc[6];
    cc[VSTART] = musl_cc[8];
    cc[VSTOP] = musl_cc[9];
    cc[VSUSP] = musl_cc[10];
    cc[VEOL] = musl_cc[11];
    cc[VREPRINT] = musl_cc[12];
    cc[VDISCARD] = musl_cc[13];
    cc[VWERASE] = musl_cc[14];
    cc[VLNEXT] = musl_cc[15];
    cc[VEOL2] = musl_cc[16];
}

static unsigned to_musl_baud(speed_t baud)
{
    switch (baud) {
    case B0:
        return 0;
    case B50:
        return 01;
    case B75:
        return 02;
    case B110:
        return 03;
    case B134:
        return 04;
    case B150:
        return 05;
    case B200:
        return 06;
    case B300:
        return 07;
    case B600:
        return 010;
    case B1200:
        return 011;
    case B1800:
        return 012;
    case B2400:
        return 013;
    case B4800:
        return 014;
    case B9600:
        return 015;
    case B19200:
        return 016;
    case B38400:
        return 017;
    case B57600:
        return 010001;
    case B115200:
        return 010002;
    case B230400:
        return 010003;
    default:
        PAS_ASSERT(!"Should not be reached");
        return 0;
    }
}

static bool from_musl_baud(unsigned musl_baud, speed_t* result)
{
    switch (musl_baud) {
    case 0:
        *result = B0;
        return true;
    case 01:
        *result = B50;
        return true;
    case 02:
        *result = B75;
        return true;
    case 03:
        *result = B110;
        return true;
    case 04:
        *result = B134;
        return true;
    case 05:
        *result = B150;
        return true;
    case 06:
        *result = B200;
        return true;
    case 07:
        *result = B300;
        return true;
    case 010:
        *result = B600;
        return true;
    case 011:
        *result = B1200;
        return true;
    case 012:
        *result = B1800;
        return true;
    case 013:
        *result = B2400;
        return true;
    case 014:
        *result = B4800;
        return true;
    case 015:
        *result = B9600;
        return true;
    case 016:
        *result = B19200;
        return true;
    case 017:
        *result = B38400;
        return true;
    case 010001:
        *result = B57600;
        return true;
    case 010002:
        *result = B115200;
        return true;
    case 010003:
        *result = B230400;
        return true;
    default:
        return false;
    }
}

static void to_musl_termios(struct termios* termios, struct musl_termios* musl_termios)
{
    musl_termios->c_iflag = to_musl_ciflag(termios->c_iflag);
    musl_termios->c_oflag = to_musl_coflag(termios->c_oflag);
    musl_termios->c_cflag = to_musl_ccflag(termios->c_cflag);
    musl_termios->c_lflag = to_musl_clflag(termios->c_lflag);
    to_musl_ccc(termios->c_cc, musl_termios->c_cc);
    musl_termios->c_ispeed = to_musl_baud(termios->c_ispeed);
    musl_termios->c_ospeed = to_musl_baud(termios->c_ospeed);
}

static bool from_musl_termios(struct musl_termios* musl_termios, struct termios* termios)
{
    if (!from_musl_ciflag(musl_termios->c_iflag, &termios->c_iflag))
        return false;
    if (!from_musl_coflag(musl_termios->c_oflag, &termios->c_oflag))
        return false;
    if (!from_musl_ccflag(musl_termios->c_cflag, &termios->c_cflag))
        return false;
    if (!from_musl_clflag(musl_termios->c_lflag, &termios->c_lflag))
        return false;
    from_musl_ccc(musl_termios->c_cc, termios->c_cc);
    if (!from_musl_baud(musl_termios->c_ispeed, &termios->c_ispeed))
        return false;
    if (!from_musl_baud(musl_termios->c_ospeed, &termios->c_ospeed))
        return false;
    return true;
}

struct musl_winsize {
    unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};

static int zsys_ioctl_impl(int fd, unsigned long request, filc_ptr args)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_ioctl_impl",
        .line = 0,
        .column = 0
    };

    int my_errno;
    
    // NOTE: This must use musl's ioctl numbers, and must treat the passed-in struct as having the
    // pizlonated musl layout.
    switch (request) {
    case 0x5401: /* TCGETS */ {
        struct termios termios;
        struct musl_termios* musl_termios;
        filc_ptr musl_termios_ptr;
        musl_termios_ptr = filc_ptr_get_next_ptr(&args, &origin);
        filc_check_access_int(musl_termios_ptr, sizeof(struct musl_termios), &origin);
        musl_termios = (struct musl_termios*)filc_ptr_ptr(musl_termios_ptr);
        filc_exit();
        if (tcgetattr(fd, &termios) < 0)
            goto error;
        filc_enter();
        to_musl_termios(&termios, musl_termios);
        return 0;
    }
    case 0x5402: /* TCGETS */
    case 0x5403: /* TCGETSW */
    case 0x5404: /* TCGETSF */ {
        struct termios termios;
        struct musl_termios* musl_termios;
        filc_ptr musl_termios_ptr;
        musl_termios_ptr = filc_ptr_get_next_ptr(&args, &origin);
        filc_check_access_int(musl_termios_ptr, sizeof(struct musl_termios), &origin);
        musl_termios = (struct musl_termios*)filc_ptr_ptr(musl_termios_ptr);
        if (!from_musl_termios(musl_termios, &termios)) {
            errno = EINVAL;
            goto error;
        }
        int optional_actions;
        switch (request) {
        case 0x5402:
            optional_actions = TCSANOW;
            break;
        case 0x5403:
            optional_actions = TCSADRAIN;
            break;
        case 0x5404:
            optional_actions = TCSAFLUSH;
            break;
        default:
            PAS_ASSERT(!"Should not be reached");
            optional_actions = 0;
            break;
        }
        filc_exit();
        if (tcsetattr(fd, optional_actions, &termios) < 0)
            goto error;
        filc_enter();
        return 0;
    }
    case 0x5413: /* TIOCGWINSZ */ {
        filc_ptr musl_winsize_ptr;
        struct musl_winsize* musl_winsize;
        struct winsize winsize;
        musl_winsize_ptr = filc_ptr_get_next_ptr(&args, &origin);
        filc_check_access_int(musl_winsize_ptr, sizeof(struct musl_winsize), &origin);
        musl_winsize = (struct musl_winsize*)filc_ptr_ptr(musl_winsize_ptr);
        filc_exit();
        if (ioctl(fd, TIOCGWINSZ, &winsize) < 0)
            goto error;
        filc_enter();
        musl_winsize->ws_row = winsize.ws_row;
        musl_winsize->ws_col = winsize.ws_col;
        musl_winsize->ws_xpixel = winsize.ws_xpixel;
        musl_winsize->ws_ypixel = winsize.ws_ypixel;
        return 0;
    }
    default:
        if (verbose)
            pas_log("Invalid ioctl request = %lu.\n", request);
        set_errno(EINVAL);
        return -1;
    }

error:
    my_errno = errno;
    if (verbose)
        pas_log("failed ioctl: %s\n", strerror(my_errno));
    filc_enter();
    set_errno(my_errno);
    return -1;
}

void pizlonated_f_zsys_ioctl(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_ioctl",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    unsigned long request = filc_ptr_get_next_unsigned_long(&args, &origin);
    int result = zsys_ioctl_impl(fd, request, args);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    *(int*)filc_ptr_ptr(rets) = result;
}

struct musl_iovec { filc_ptr iov_base; size_t iov_len; };

static struct iovec* prepare_iovec(filc_ptr musl_iov, int iovcnt)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "prepare_iovec",
        .line = 0,
        .column = 0
    };
    struct iovec* iov;
    size_t iov_size;
    size_t index;
    FILC_CHECK(
        iovcnt >= 0,
        &origin,
        "iovcnt cannot be negative; iovcnt = %d.\n", iovcnt);
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(struct iovec), iovcnt, &iov_size),
        &origin,
        "iovcnt too large, leading to overflow; iovcnt = %d.\n", iovcnt);
    iov = filc_allocate_utility(iov_size);
    for (index = 0; index < (size_t)iovcnt; ++index) {
        filc_ptr musl_iov_entry;
        filc_ptr musl_iov_base;
        size_t iov_len;
        musl_iov_entry = filc_ptr_with_offset(musl_iov, sizeof(struct musl_iovec) * index);
        filc_check_access_ptr(
            filc_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_base)),
            &origin);
        filc_check_access_int(
            filc_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_len)),
            sizeof(size_t), &origin);
        musl_iov_base = ((struct musl_iovec*)filc_ptr_ptr(musl_iov_entry))->iov_base;
        iov_len = ((struct musl_iovec*)filc_ptr_ptr(musl_iov_entry))->iov_len;
        filc_check_access_int(musl_iov_base, iov_len, &origin);
        iov[index].iov_base = filc_ptr_ptr(musl_iov_base);
        iov[index].iov_len = iov_len;
    }
    return iov;
}

void pizlonated_f_zsys_writev(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_writev",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_iov = filc_ptr_get_next_ptr(&args, &origin);
    int iovcnt = filc_ptr_get_next_int(&args, &origin);
    ssize_t result;
    PIZLONATED_DELETE_ARGS();
    struct iovec* iov = prepare_iovec(musl_iov, iovcnt);
    filc_exit();
    result = writev(fd, iov, iovcnt);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_deallocate(iov);
    filc_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_read(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_read",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr buf = filc_ptr_get_next_ptr(&args, &origin);
    ssize_t size = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(buf, size, &origin);
    filc_exit();
    int result = read(fd, filc_ptr_ptr(buf), size);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_readv(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_readv",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_iov = filc_ptr_get_next_ptr(&args, &origin);
    int iovcnt = filc_ptr_get_next_int(&args, &origin);
    ssize_t result;
    PIZLONATED_DELETE_ARGS();
    struct iovec* iov = prepare_iovec(musl_iov, iovcnt);
    filc_exit();
    result = readv(fd, iov, iovcnt);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_deallocate(iov);
    filc_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_write(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_write",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr buf = filc_ptr_get_next_ptr(&args, &origin);
    ssize_t size = filc_ptr_get_next_size_t(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(buf, size, &origin);
    filc_exit();
    int result = write(fd, filc_ptr_ptr(buf), size);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_check_access_int(rets, sizeof(ssize_t), &origin);
    *(ssize_t*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_close(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_close",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_exit();
    int result = close(fd);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_check_access_int(rets, sizeof(int), &origin);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_lseek(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_lseek",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    long offset = filc_ptr_get_next_long(&args, &origin);
    int whence = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_exit();
    long result = lseek(fd, offset, whence);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_check_access_int(rets, sizeof(long), &origin);
    *(long*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_exit(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_exit",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    int return_code = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_exit();
    _exit(return_code);
    PAS_ASSERT(!"Should not be reached");
}

void pizlonated_f_zsys_getuid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getuid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    filc_exit();
    unsigned result = getuid();
    filc_enter();
    *(unsigned*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_geteuid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_geteuid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    filc_exit();
    unsigned result = geteuid();
    filc_enter();
    *(unsigned*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_getgid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getgid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    filc_exit();
    unsigned result = getgid();
    filc_enter();
    *(unsigned*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_getegid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getegid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    filc_exit();
    unsigned result = getegid();
    filc_enter();
    *(unsigned*)filc_ptr_ptr(rets) = result;
}

static int from_musl_open_flags(int musl_flags)
{
    int result = 0;

    if (check_and_clear(&musl_flags, 01))
        result |= O_WRONLY;
    if (check_and_clear(&musl_flags, 02))
        result |= O_RDWR;
    if (check_and_clear(&musl_flags, 0100))
        result |= O_CREAT;
    if (check_and_clear(&musl_flags, 0200))
        result |= O_EXCL;
    if (check_and_clear(&musl_flags, 0400))
        result |= O_NOCTTY;
    if (check_and_clear(&musl_flags, 01000))
        result |= O_TRUNC;
    if (check_and_clear(&musl_flags, 02000))
        result |= O_APPEND;
    if (check_and_clear(&musl_flags, 04000))
        result |= O_NONBLOCK;
    if (check_and_clear(&musl_flags, 0200000))
        result |= O_DIRECTORY;
    if (check_and_clear(&musl_flags, 0400000))
        result |= O_NOFOLLOW;
    if (check_and_clear(&musl_flags, 02000000))
        result |= O_CLOEXEC;
    if (check_and_clear(&musl_flags, 020000))
        result |= O_ASYNC;

    if (musl_flags)
        return -1;
    return result;
}

static int to_musl_open_flags(int flags)
{
    int result = 0;

    if (check_and_clear(&flags, O_WRONLY))
        result |= 01;
    if (check_and_clear(&flags, O_RDWR))
        result |= 02;
    if (check_and_clear(&flags, O_CREAT))
        result |= 0100;
    if (check_and_clear(&flags, O_EXCL))
        result |= 0200;
    if (check_and_clear(&flags, O_NOCTTY))
        result |= 0400;
    if (check_and_clear(&flags, O_TRUNC))
        result |= 01000;
    if (check_and_clear(&flags, O_APPEND))
        result |= 02000;
    if (check_and_clear(&flags, O_NONBLOCK))
        result |= 04000;
    if (check_and_clear(&flags, O_DIRECTORY))
        result |= 0200000;
    if (check_and_clear(&flags, O_NOFOLLOW))
        result |= 0400000;
    if (check_and_clear(&flags, O_CLOEXEC))
        result |= 02000000;
    if (check_and_clear(&flags, O_ASYNC))
        result |= 020000;

    /* Fun fact: on MacOS, I get an additional 0x10000 flag, and I don't know what it is.
       Ima just ignore it and hope for the best LOL! */
    PAS_ASSERT(!(flags & ~0x10000));
    
    return result;
}

void pizlonated_f_zsys_open(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_open",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr path_ptr = filc_ptr_get_next_ptr(&args, &origin);
    int musl_flags = filc_ptr_get_next_int(&args, &origin);
    int flags = from_musl_open_flags(musl_flags);
    int mode = 0;
    if (flags >= 0 && (flags & O_CREAT))
        mode = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    if (flags < 0) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    const char* path = filc_check_and_get_new_str(path_ptr, &origin);
    filc_exit();
    int result = open(path, flags, mode);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_deallocate(path);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_getpid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int result = getpid();
    filc_enter();
    *(int*)filc_ptr_ptr(rets) = result;
}

static bool from_musl_clock_id(int musl_clock_id, clockid_t* result)
{
    switch (musl_clock_id) {
    case 0:
        *result = CLOCK_REALTIME;
        return true;
    case 1:
        *result = CLOCK_MONOTONIC;
        return true;
    case 2:
        *result = CLOCK_PROCESS_CPUTIME_ID;
        return true;
    case 3:
        *result = CLOCK_THREAD_CPUTIME_ID;
        return true;
    case 4:
        *result = CLOCK_MONOTONIC_RAW;
        return true;
    default:
        *result = 0;
        return false;
    }
}

struct musl_timespec {
    uint64_t tv_sec;
    uint64_t tv_nsec;
};

void pizlonated_f_zsys_clock_gettime(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_clock_gettime",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_clock_id = filc_ptr_get_next_int(&args, &origin);
    filc_ptr timespec_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(timespec_ptr, sizeof(struct musl_timespec), &origin);
    clockid_t clock_id;
    if (!from_musl_clock_id(musl_clock_id, &clock_id)) {
        *(int*)filc_ptr_ptr(rets) = -1;
        set_errno(EINVAL);
        return;
    }
    struct timespec ts;
    filc_exit();
    int result = clock_gettime(clock_id, &ts);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        *(int*)filc_ptr_ptr(rets) = -1;
        set_errno(my_errno);
        return;
    }
    struct musl_timespec* musl_timespec = (struct musl_timespec*)filc_ptr_ptr(timespec_ptr);
    musl_timespec->tv_sec = ts.tv_sec;
    musl_timespec->tv_nsec = ts.tv_nsec;
}

struct musl_stat {
    uint64_t st_dev;
    uint64_t st_ino;
    unsigned st_mode;
    uint64_t st_nlink;
    unsigned st_uid;
    unsigned st_gid;
    uint64_t st_rdev;
    int64_t st_size;
    long st_blksize;
    int64_t st_blocks;
    uint64_t st_atim[2];
    uint64_t st_mtim[2];
    uint64_t st_ctim[2];
};

static bool from_musl_fstatat_flag(int musl_flag, int* result)
{
    *result = 0;
    if (check_and_clear(&musl_flag, 0x100))
        *result |= AT_SYMLINK_NOFOLLOW;
    if (check_and_clear(&musl_flag, 0x200))
        *result |= AT_REMOVEDIR | AT_EACCESS;
    if (check_and_clear(&musl_flag, 0x400))
        *result |= AT_SYMLINK_FOLLOW;
    return !musl_flag;
}

static void handle_fstat_result(filc_ptr rets, filc_ptr musl_stat_ptr, struct stat *st,
                                int result, int my_errno)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "handle_fstat_result",
        .line = 0,
        .column = 0
    };
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_stat_ptr, sizeof(struct musl_stat), &origin);
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    struct musl_stat* musl_stat = (struct musl_stat*)filc_ptr_ptr(musl_stat_ptr);
    musl_stat->st_dev = st->st_dev;
    musl_stat->st_ino = st->st_ino;
    musl_stat->st_mode = st->st_mode;
    musl_stat->st_nlink = st->st_nlink;
    musl_stat->st_uid = st->st_uid;
    musl_stat->st_gid = st->st_gid;
    musl_stat->st_rdev = st->st_rdev;
    musl_stat->st_size = st->st_size;
    musl_stat->st_blksize = st->st_blksize;
    musl_stat->st_blocks = st->st_blocks;
    musl_stat->st_atim[0] = st->st_atimespec.tv_sec;
    musl_stat->st_atim[1] = st->st_atimespec.tv_nsec;
    musl_stat->st_mtim[0] = st->st_mtimespec.tv_sec;
    musl_stat->st_mtim[1] = st->st_mtimespec.tv_nsec;
    musl_stat->st_ctim[0] = st->st_ctimespec.tv_sec;
    musl_stat->st_ctim[1] = st->st_ctimespec.tv_nsec;
}

void pizlonated_f_zsys_fstatat(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fstatat",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr path_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_stat_ptr = filc_ptr_get_next_ptr(&args, &origin);
    int musl_flag = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    int flag;
    if (!from_musl_fstatat_flag(musl_flag, &flag)) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    if (fd == -100)
        fd = AT_FDCWD;
    struct stat st;
    const char* path = filc_check_and_get_new_str(path_ptr, &origin);
    filc_exit();
    int result = fstatat(fd, path, &st, flag);
    int my_errno = errno;
    filc_enter();
    handle_fstat_result(rets, musl_stat_ptr, &st, result, my_errno);
    filc_deallocate(path);
}

void pizlonated_f_zsys_fstat(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fstat",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_stat_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    struct stat st;
    filc_exit();
    int result = fstat(fd, &st);
    int my_errno = errno;
    filc_enter();
    handle_fstat_result(rets, musl_stat_ptr, &st, result, my_errno);
}

struct musl_flock {
    short l_type;
    short l_whence;
    int64_t l_start;
    int64_t l_len;
    int l_pid;
};

void pizlonated_f_zsys_fcntl(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fcntl",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    int musl_cmd = filc_ptr_get_next_int(&args, &origin);
    filc_check_access_int(rets, sizeof(int), &origin);
    int cmd = 0;
    bool have_cmd = true;
    bool have_arg_int = false;
    bool have_arg_flock = false;
    switch (musl_cmd) {
    case 0:
        cmd = F_DUPFD;
        have_arg_int = true;
        break;
    case 1:
        cmd = F_GETFD;
        break;
    case 2:
        cmd = F_SETFD;
        have_arg_int = true;
        break;
    case 3:
        cmd = F_GETFL;
        if (verbose)
            pas_log("F_GETFL!\n");
        break;
    case 4:
        cmd = F_SETFL;
        have_arg_int = true;
        break;
    case 8:
        cmd = F_SETOWN;
        have_arg_int = true;
        break;
    case 9:
        cmd = F_GETOWN;
        break;
    case 5:
        cmd = F_GETLK;
        have_arg_flock = true;
        break;
    case 6:
        cmd = F_SETLK;
        have_arg_flock = true;
        break;
    case 7:
        cmd = F_SETLKW;
        have_arg_flock = true;
        break;
    case 1030:
        cmd = F_DUPFD_CLOEXEC;
        have_arg_int = true;
        break;
    default:
        have_cmd = false;
        break;
    }
    int arg_int = 0;
    struct flock arg_flock;
    filc_ptr musl_flock_ptr;
    struct musl_flock* musl_flock;
    pas_zero_memory(&arg_flock, sizeof(arg_flock));
    if (have_arg_int)
        arg_int = filc_ptr_get_next_int(&args, &origin);
    if (have_arg_flock) {
        musl_flock_ptr = filc_ptr_get_next_ptr(&args, &origin);
        filc_check_access_int(musl_flock_ptr, sizeof(struct musl_flock), &origin);
        musl_flock = (struct musl_flock*)filc_ptr_ptr(musl_flock_ptr);
        switch (musl_flock->l_type) {
        case 0:
            arg_flock.l_type = F_RDLCK;
            break;
        case 1:
            arg_flock.l_type = F_WRLCK;
            break;
        case 2:
            arg_flock.l_type = F_UNLCK;
            break;
        default:
            PIZLONATED_DELETE_ARGS();
            set_errno(EINVAL);
            *(int*)filc_ptr_ptr(rets) = -1;
            return;
        }
        arg_flock.l_whence = musl_flock->l_whence;
        arg_flock.l_start = musl_flock->l_start;
        arg_flock.l_len = musl_flock->l_len;
        arg_flock.l_pid = musl_flock->l_pid;
    }
    PIZLONATED_DELETE_ARGS();
    if (!have_cmd) {
        if (verbose)
            pas_log("no cmd!\n");
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    if (verbose)
        pas_log("so far so good.\n");
    switch (cmd) {
    case F_SETFD:
        switch (arg_int) {
        case 0:
            break;
        case 1:
            arg_int = FD_CLOEXEC;
            break;
        default:
            set_errno(EINVAL);
            *(int*)filc_ptr_ptr(rets) = -1;
            return;
        }
        break;
    case F_SETFL:
        arg_int = from_musl_open_flags(arg_int);
        break;
    default:
        break;
    }
    int result;
    filc_exit();
    if (have_arg_int) {
        PAS_ASSERT(!have_arg_flock);
        result = fcntl(fd, cmd, arg_int);
    } else if (have_arg_flock) {
        struct flock saved_flock = arg_flock;
        result = fcntl(fd, cmd, &arg_flock);
        if (arg_flock.l_type != saved_flock.l_type) {
            switch (arg_flock.l_type) {
            case F_RDLCK:
                musl_flock->l_type = 0;
                break;
            case F_WRLCK:
                musl_flock->l_type = 1;
                break;
            case F_UNLCK:
                musl_flock->l_type = 2;
                break;
            default:
                /* WTF? */
                musl_flock->l_type = arg_flock.l_type;
                break;
            }
        }
#define SET_IF_DIFFERENT(field) do { \
        if (arg_flock.field != saved_flock.field) \
            musl_flock->field = arg_flock.field; \
    } while (false)
        SET_IF_DIFFERENT(l_whence);
        SET_IF_DIFFERENT(l_start);
        SET_IF_DIFFERENT(l_len);
        SET_IF_DIFFERENT(l_pid);
#undef SET_IF_DIFFERENT
    } else
        result = fcntl(fd, cmd);
    int my_errno = errno;
    filc_enter();
    if (verbose)
        pas_log("result = %d\n", result);
    if (result == -1) {
        if (verbose)
            pas_log("error = %s\n", strerror(my_errno));
        set_errno(my_errno);
    }
    switch (cmd) {
    case F_GETFD:
        switch (result) {
        case 0:
            break;
        case FD_CLOEXEC:
            result = 1;
            break;
        default:
            /* WTF? */
            break;
        }
        break;
    case F_GETFL:
        result = to_musl_open_flags(result);
        break;
    default:
        break;
    }
    *(int*)filc_ptr_ptr(rets) = result;
}

static struct musl_passwd* to_musl_passwd_threadlocal(struct passwd* passwd)
{
    struct musl_passwd* result = (struct musl_passwd*)pthread_getspecific(musl_passwd_threadlocal_key);
    if (result)
        musl_passwd_free_guts(result);
    else {
        result = filc_allocate_one(&musl_passwd_heap);
        pthread_setspecific(musl_passwd_threadlocal_key, result);
    }
    filc_low_level_ptr_safe_bzero(result, sizeof(struct musl_passwd));

    filc_ptr_store(&result->pw_name, filc_strdup(passwd->pw_name));
    filc_ptr_store(&result->pw_passwd, filc_strdup(passwd->pw_passwd));
    result->pw_uid = passwd->pw_uid;
    result->pw_gid = passwd->pw_gid;
    filc_ptr_store(&result->pw_gecos, filc_strdup(passwd->pw_gecos));
    filc_ptr_store(&result->pw_dir, filc_strdup(passwd->pw_dir));
    filc_ptr_store(&result->pw_shell, filc_strdup(passwd->pw_shell));
    return result;
}

void pizlonated_f_zsys_getpwuid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpwuid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    unsigned uid = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct passwd* passwd = getpwuid(uid);
    if (!passwd) {
        set_errno(errno);
        return;
    }
    struct musl_passwd* musl_passwd = to_musl_passwd_threadlocal(passwd);
    *(filc_ptr*)filc_ptr_ptr(rets) =
        filc_ptr_forge(musl_passwd, musl_passwd, musl_passwd + 1, &musl_passwd_type);
}

static void from_musl_sigset(struct musl_sigset* musl_sigset,
                             sigset_t* sigset)
{
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    sigfillset(sigset);
    
    unsigned musl_signum;
    for (musl_signum = num_active_bits; musl_signum--;) {
        bool bit_value = !!(musl_sigset->bits[PAS_BITVECTOR_WORD64_INDEX(musl_signum)]
                            & PAS_BITVECTOR_BIT_MASK64(musl_signum));
        if (bit_value)
            continue;
        int signum = from_musl_signum(musl_signum);
        if (signum < 0)
            continue;
        sigdelset(sigset, signum);
    }
}

static bool from_musl_sa_flags(int musl_flags, int* flags)
{
    *flags = 0;
    /* NOTE: We explicitly exclude SA_SIGINFO because we do not support it yet!! */
    if (check_and_clear(&musl_flags, 1))
        *flags |= SA_NOCLDSTOP;
    if (check_and_clear(&musl_flags, 2))
        *flags |= SA_NOCLDWAIT;
    if (check_and_clear(&musl_flags, 0x08000000))
        *flags |= SA_ONSTACK;
    if (check_and_clear(&musl_flags, 0x10000000))
        *flags |= SA_RESTART;
    if (check_and_clear(&musl_flags, 0x40000000))
        *flags |= SA_NODEFER;
    if (check_and_clear(&musl_flags, 0x80000000))
        *flags |= SA_RESETHAND;
    return !musl_flags;
}

static void to_musl_sigset(sigset_t* sigset, struct musl_sigset* musl_sigset)
{
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    memset(musl_sigset, 255, sizeof(struct musl_sigset));
    
    unsigned musl_signum;
    for (musl_signum = num_active_bits; musl_signum--;) {
        int signum = from_musl_signum(musl_signum);
        if (signum < 0)
            continue;
        if (!sigismember(sigset, signum)) {
            musl_sigset->bits[PAS_BITVECTOR_WORD64_INDEX(musl_signum)] &=
                ~PAS_BITVECTOR_BIT_MASK64(musl_signum);
        }
    }
}

static int to_musl_sa_flags(int sa_flags)
{
    int result = 0;
    if (check_and_clear(&sa_flags, SA_NOCLDSTOP))
        result |= 1;
    if (check_and_clear(&sa_flags, SA_NOCLDWAIT))
        result |= 2;
    if (check_and_clear(&sa_flags, SA_SIGINFO))
        result |= 4;
    if (check_and_clear(&sa_flags, SA_ONSTACK))
        result |= 0x08000000;
    if (check_and_clear(&sa_flags, SA_RESTART))
        result |= 0x10000000;
    if (check_and_clear(&sa_flags, SA_NODEFER))
        result |= 0x40000000;
    if (check_and_clear(&sa_flags, SA_RESETHAND))
        result |= 0x80000000;
    PAS_ASSERT(!sa_flags);
    return result;
}

static void check_signal(int signum, const filc_origin* origin)
{
    FILC_CHECK(signum != SIGILL, origin, "cannot override SIGILL.");
    FILC_CHECK(signum != SIGTRAP, origin, "cannot override SIGTRAP.");
    FILC_CHECK(signum != SIGBUS, origin, "cannot override SIGBUS.");
    FILC_CHECK(signum != SIGSEGV, origin, "cannot override SIGSEGV.");
    FILC_CHECK(signum != SIGFPE, origin, "cannot override SIGFPE.");
}

static bool is_musl_special_signal_handler(void* handler)
{
    return handler == NULL || handler == (void*)(uintptr_t)1;
}

static void* from_musl_special_signal_handler(void* handler)
{
    PAS_ASSERT(is_musl_special_signal_handler(handler));
    return handler == NULL ? SIG_DFL : SIG_IGN;
}

static bool is_special_signal_handler(void* handler)
{
    return handler == SIG_DFL || handler == SIG_IGN;
}

static filc_ptr to_musl_special_signal_handler(void* handler)
{
    if (handler == SIG_DFL)
        return filc_ptr_forge_invalid(NULL);
    if (handler == SIG_IGN)
        return filc_ptr_forge_invalid((void*)(uintptr_t)1);
    PAS_ASSERT(!"Bad special handler");
    return filc_ptr_forge_invalid(NULL);
}

void pizlonated_f_zsys_sigaction(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_sigaction",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_signum = filc_ptr_get_next_int(&args, &origin);
    filc_ptr act_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr oact_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    int signum = from_musl_signum(musl_signum);
    if (signum < 0) {
        if (verbose)
            pas_log("bad signum\n");
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    check_signal(signum, &origin);
    if (filc_ptr_ptr(act_ptr))
        filc_restrict(act_ptr, 1, &musl_sigaction_type, &origin);
    if (filc_ptr_ptr(oact_ptr))
        filc_restrict(oact_ptr, 1, &musl_sigaction_type, &origin);
    struct musl_sigaction* musl_act = (struct musl_sigaction*)filc_ptr_ptr(act_ptr);
    struct musl_sigaction* musl_oact = (struct musl_sigaction*)filc_ptr_ptr(oact_ptr);
    struct sigaction act;
    struct sigaction oact;
    if (musl_act) {
        from_musl_sigset(&musl_act->sa_mask, &act.sa_mask);
        filc_ptr musl_handler = filc_ptr_load(&musl_act->sa_handler_ish);
        if (is_musl_special_signal_handler(filc_ptr_ptr(musl_handler)))
            act.sa_handler = from_musl_special_signal_handler(filc_ptr_ptr(musl_handler));
        else {
            filc_check_function_call(musl_handler, &origin);
            signal_handler* handler = filc_allocate_utility(sizeof(signal_handler));
            handler->function = filc_ptr_ptr(musl_handler);
            handler->mask = act.sa_mask;
            handler->musl_signum = musl_signum;
            pas_store_store_fence();
            PAS_ASSERT((unsigned)musl_signum <= MAX_MUSL_SIGNUM);
            signal_table[musl_signum] = handler;
            act.sa_handler = signal_pizlonator;
        }
        if (!from_musl_sa_flags(musl_act->sa_flags, &act.sa_flags)) {
            set_errno(EINVAL);
            *(int*)filc_ptr_ptr(rets) = -1;
            return;
        }
    }
    if (musl_oact)
        pas_zero_memory(&oact, sizeof(struct sigaction));
    filc_exit();
    int result = sigaction(signum, musl_act ? &act : NULL, musl_oact ? &oact : NULL);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = 1;
        return;
    }
    if (musl_oact) {
        if (is_special_signal_handler(oact.sa_handler))
            filc_ptr_store(&musl_oact->sa_handler_ish, to_musl_special_signal_handler(oact.sa_handler));
        else {
            PAS_ASSERT(oact.sa_handler == signal_pizlonator);
            PAS_ASSERT((unsigned)musl_signum <= MAX_MUSL_SIGNUM);
            filc_ptr_store(&musl_oact->sa_handler_ish,
                           filc_ptr_forge_byte(signal_table[musl_signum], &filc_function_type));
        }
        to_musl_sigset(&oact.sa_mask, &musl_oact->sa_mask);
        musl_oact->sa_flags = to_musl_sa_flags(oact.sa_flags);
    }
}

void pizlonated_f_zsys_isatty(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_isatty",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    errno = 0;
    int result = isatty(fd);
    int my_errno = errno;
    filc_enter();
    if (!result && my_errno)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_pipe(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_pipe",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr fds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(fds_ptr, sizeof(int) * 2, &origin);
    int fds[2];
    filc_exit();
    int result = pipe(fds);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        /* Make sure not to modify what fds_ptr points to on error, even if the system modified
           our fds, since that would be nonconforming behavior. Probably doesn't matter since of
           course we would never run on a nonconforming system. */
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    ((int*)filc_ptr_ptr(fds_ptr))[0] = fds[0];
    ((int*)filc_ptr_ptr(fds_ptr))[1] = fds[1];
}

struct musl_timeval {
    uint64_t tv_sec;
    uint64_t tv_usec;
};

void pizlonated_f_zsys_select(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_select",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int nfds = filc_ptr_get_next_int(&args, &origin);
    filc_ptr readfds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr writefds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr exceptfds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr timeout_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    PAS_ASSERT(FD_SETSIZE == 1024);
    FILC_CHECK(
        nfds <= 1024,
        &origin,
        "attempt to select with nfds = %d (should be 1024 or less).",
        nfds);
    if (filc_ptr_ptr(readfds_ptr))
        filc_check_access_int(readfds_ptr, sizeof(fd_set), &origin);
    if (filc_ptr_ptr(writefds_ptr))
        filc_check_access_int(writefds_ptr, sizeof(fd_set), &origin);
    if (filc_ptr_ptr(exceptfds_ptr))
        filc_check_access_int(exceptfds_ptr, sizeof(fd_set), &origin);
    if (filc_ptr_ptr(timeout_ptr))
        filc_check_access_int(timeout_ptr, sizeof(struct musl_timeval), &origin);
    fd_set* readfds = (fd_set*)filc_ptr_ptr(readfds_ptr);
    fd_set* writefds = (fd_set*)filc_ptr_ptr(writefds_ptr);
    fd_set* exceptfds = (fd_set*)filc_ptr_ptr(exceptfds_ptr);
    struct musl_timeval* musl_timeout = (struct musl_timeval*)filc_ptr_ptr(timeout_ptr);
    struct timeval timeout;
    if (musl_timeout) {
        timeout.tv_sec = musl_timeout->tv_sec;
        timeout.tv_usec = musl_timeout->tv_usec;
    }
    filc_exit();
    int result = select(nfds, readfds, writefds, exceptfds, musl_timeout ? &timeout : NULL);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    filc_check_access_int(rets, sizeof(int), &origin);
    *(int*)filc_ptr_ptr(rets) = result;
    if (musl_timeout) {
        musl_timeout->tv_sec = timeout.tv_sec;
        musl_timeout->tv_usec = timeout.tv_usec;
    }
}

void pizlonated_f_zsys_sched_yield(PIZLONATED_SIGNATURE)
{
    PIZLONATED_DELETE_ARGS();
    filc_exit();
    sched_yield();
    filc_enter();
}

static bool from_musl_domain(int musl_domain, int* result)
{
    switch (musl_domain) {
    case 0:
        *result = PF_UNSPEC;
        return true;
    case 1:
        *result = PF_LOCAL;
        return true;
    case 2:
        *result = PF_INET;
        return true;
    case 4:
        *result = PF_IPX;
        return true;
    case 5:
        *result = PF_APPLETALK;
        return true;
    case 10:
        *result = PF_INET6;
        return true;
    case 12:
        *result = PF_DECnet;
        return true;
    case 15:
        *result = PF_KEY;
        return true;
    case 16:
        *result = PF_ROUTE;
        return true;
    case 22:
        *result = PF_SNA;
        return true;
    case 34:
        *result = PF_ISDN;
        return true;
    case 40:
        *result = PF_VSOCK;
        return true;
    default:
        return false;
    }
}

static bool to_musl_domain(int domain, int* result)
{
    switch (domain) {
    case PF_UNSPEC:
        *result = 0;
        return true;
    case PF_LOCAL:
        *result = 1;
        return true;
    case PF_INET:
        *result = 2;
        return true;
    case PF_IPX:
        *result = 4;
        return true;
    case PF_APPLETALK:
        *result = 5;
        return true;
    case PF_INET6:
        *result = 10;
        return true;
    case PF_DECnet:
        *result = 12;
        return true;
    case PF_KEY:
        *result = 15;
        return true;
    case PF_ROUTE:
        *result = 16;
        return true;
    case PF_SNA:
        *result = 22;
        return true;
    case PF_ISDN:
        *result = 34;
        return true;
    case PF_VSOCK:
        *result = 40;
        return true;
    default:
        return false;
    }
}

static bool from_musl_socket_type(int musl_type, int* result)
{
    switch (musl_type) {
    case 1:
        *result = SOCK_STREAM;
        return true;
    case 2:
        *result = SOCK_DGRAM;
        return true;
    case 3:
        *result = SOCK_RAW;
        return true;
    case 4:
        *result = SOCK_RDM;
        return true;
    case 5:
        *result = SOCK_SEQPACKET;
        return true;
    default:
        return false;
    }
}

static bool to_musl_socket_type(int type, int* result)
{
    switch (type) {
    case SOCK_STREAM:
        *result = 1;
        return true;
    case SOCK_DGRAM:
        *result = 2;
        return true;
    case SOCK_RAW:
        *result = 3;
        return true;
    case SOCK_RDM:
        *result = 4;
        return true;
    case SOCK_SEQPACKET:
        *result = 5;
        return true;
    default:
        return false;
    }
}

void pizlonated_f_zsys_socket(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_socket",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_domain = filc_ptr_get_next_int(&args, &origin);
    int musl_type = filc_ptr_get_next_int(&args, &origin);
    int protocol = filc_ptr_get_next_int(&args, &origin); /* these constants seem to align between
                                                               Darwin and musl. */
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    int domain;
    if (!from_musl_domain(musl_domain, &domain)) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    int type;
    if (!from_musl_socket_type(musl_type, &type)) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    filc_exit();
    int result = socket(domain, type, protocol);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

/* The socket level is different from the protocol in that SOL_SOCKET can be passed, and oddly,
   in Linux it shares the same constant as IPPROTO_ICMP. */
static bool from_musl_socket_level(int musl_level, int* result)
{
    if (musl_level == 1)
        *result = SOL_SOCKET;
    else
        *result = musl_level;
    return true;
}

static bool from_musl_so_optname(int musl_optname, int* result)
{
    switch (musl_optname) {
    case 2:
        *result = SO_REUSEADDR;
        return true;
    case 3:
        *result = SO_TYPE;
        return true;
    case 4:
        *result = SO_ERROR;
        return true;
    case 7:
        *result = SO_SNDBUF;
        return true;
    case 8:
        *result = SO_RCVBUF;
        return true;
    case 9:
        *result = SO_KEEPALIVE;
        return true;
    case 15:
        *result = SO_REUSEPORT;
        return true;
    case 20:
        *result = SO_RCVTIMEO;
        return true;
    default:
        return false;
    }
}

static bool from_musl_ip_optname(int musl_optname, int* result)
{
    switch (musl_optname) {
    case 1:
        *result = IP_TOS;
        return true;
    case 4:
        *result = IP_OPTIONS;
        return true;
    default:
        return false;
    }
}

static bool from_musl_ipv6_optname(int musl_optname, int* result)
{
    switch (musl_optname) {
    case 26:
        *result = IPV6_V6ONLY;
        return true;
    case 67:
        *result = IPV6_TCLASS;
        return true;
    default:
        return false;
    }
}

static bool from_musl_tcp_optname(int musl_optname, int* result)
{
    switch (musl_optname) {
    case 1:
        *result = TCP_NODELAY;
        return true;
    case 4:
        *result = TCP_KEEPALIVE; /* musl says KEEPIDLE, Darwin says KEEPALIVE. coooool. */
        return true;
    case 5:
        *result = TCP_KEEPINTVL;
        return true;
    default:
        return false;
    }
}

void pizlonated_f_zsys_setsockopt(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_setsockopt",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    int musl_level = filc_ptr_get_next_int(&args, &origin);
    int musl_optname = filc_ptr_get_next_int(&args, &origin);
    filc_ptr optval_ptr = filc_ptr_get_next_ptr(&args, &origin);
    unsigned optlen = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    int level;
    if (!from_musl_socket_level(musl_level, &level))
        goto einval;
    int optname;
    /* for now, all of the possible arguments are primitives. */
    filc_check_access_int(optval_ptr, optlen, &origin);
    /* and most of them make sense to pass through without conversion. */
    void* optval = filc_ptr_ptr(optval_ptr);
    switch (level) {
    case SOL_SOCKET:
        if (!from_musl_so_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case SO_REUSEADDR:
        case SO_KEEPALIVE:
        case SO_SNDBUF:
        case SO_RCVBUF:
        case SO_REUSEPORT:
            break;
        case SO_RCVTIMEO: {
            if (optlen < sizeof(struct musl_timeval))
                goto einval;
            struct timeval* tv = alloca(sizeof(struct timeval));
            struct musl_timeval* musl_tv = (struct musl_timeval*)optval;
            optval = tv;
            tv->tv_sec = musl_tv->tv_sec;
            tv->tv_usec = musl_tv->tv_usec;
            break;
        }
        default:
            goto enoprotoopt;
        }
        break;
    case IPPROTO_IP:
        if (!from_musl_ip_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case IP_TOS:
        case IP_OPTIONS:
            break;
        default:
            goto enoprotoopt;
        }
        break;
    case IPPROTO_TCP:
        if (!from_musl_tcp_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case TCP_NODELAY:
        case TCP_KEEPALIVE:
        case TCP_KEEPINTVL:
            break;
        default:
            goto enoprotoopt;
        }
        break;
    case IPPROTO_IPV6:
        if (!from_musl_ipv6_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case IPV6_V6ONLY:
        case IPV6_TCLASS:
            break;
        default:
            goto enoprotoopt;
        }
        break;
    default:
        goto einval;
    }
    filc_exit();
    int result = setsockopt(sockfd, level, optname, optval, optlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
    return;
    
enoprotoopt:
    set_errno(ENOPROTOOPT);
    *(int*)filc_ptr_ptr(rets) = -1;
    return;
    
einval:
    set_errno(EINVAL);
    *(int*)filc_ptr_ptr(rets) = -1;
}

struct musl_sockaddr {
    uint16_t sa_family;
    uint8_t sa_data[14];
};

struct musl_sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    uint8_t sin_zero[8];
};

struct musl_sockaddr_in6 {
    uint16_t sin6_family;
    uint16_t sin6_port;
    uint32_t sin6_flowinfo;
    uint32_t sin6_addr[4];
    uint32_t sin6_scope_id;
};

struct musl_sockaddr_un {
    uint16_t sun_family;
    char sun_path[108];
};

#define MAX_SOCKADDRLEN PAS_MAX_CONST(PAS_MAX_CONST(sizeof(struct sockaddr), \
                                                    sizeof(struct sockaddr_in)), \
                                      PAS_MAX_CONST(sizeof(struct sockaddr_in6), \
                                                    sizeof(struct sockaddr_un)))

#define MAX_MUSL_SOCKADDRLEN PAS_MAX_CONST(PAS_MAX_CONST(sizeof(struct musl_sockaddr), \
                                                         sizeof(struct musl_sockaddr_in)), \
                                           PAS_MAX_CONST(sizeof(struct musl_sockaddr_in6), \
                                                         sizeof(struct musl_sockaddr_un)))

static bool from_musl_sockaddr(struct musl_sockaddr* musl_addr, unsigned musl_addrlen,
                               struct sockaddr** addr, unsigned* addrlen, const filc_origin* origin)
{
    static const bool verbose = false;

    if (!musl_addr) {
        PAS_ASSERT(!musl_addrlen);
        *addr = NULL;
        *addrlen = 0;
        return true;
    }

    PAS_ASSERT(musl_addrlen >= sizeof(struct musl_sockaddr));
    
    int family;
    if (!from_musl_domain(musl_addr->sa_family, &family))
        return false;
    switch (family) {
    case PF_INET: {
        if (musl_addrlen < sizeof(struct musl_sockaddr_in))
            return false;
        struct musl_sockaddr_in* musl_addr_in = (struct musl_sockaddr_in*)musl_addr;
        struct sockaddr_in* addr_in = (struct sockaddr_in*)
            filc_allocate_utility(sizeof(struct sockaddr_in));
        pas_zero_memory(addr_in, sizeof(struct sockaddr_in));
        if (verbose)
            pas_log("inet!\n");
        addr_in->sin_family = PF_INET;
        addr_in->sin_port = musl_addr_in->sin_port;
        if (verbose)
            pas_log("port = %u\n", (unsigned)addr_in->sin_port);
        addr_in->sin_addr.s_addr = musl_addr_in->sin_addr;
        if (verbose)
            pas_log("ip = %u\n", (unsigned)addr_in->sin_addr.s_addr);
        *addr = (struct sockaddr*)addr_in;
        *addrlen = sizeof(struct sockaddr_in);
        return true;
    }
    case PF_LOCAL: {
        if (musl_addrlen < sizeof(struct musl_sockaddr_un))
            return false;
        struct musl_sockaddr_un* musl_addr_un = (struct musl_sockaddr_un*)musl_addr;
        struct sockaddr_un* addr_un = (struct sockaddr_un*)
            filc_allocate_utility(sizeof(struct sockaddr_un));
        pas_zero_memory(addr_un, sizeof(struct sockaddr_un));
        addr_un->sun_family = PF_LOCAL;
        const char* sun_path = filc_check_and_get_new_str(
            filc_ptr_forge_with_size(
                musl_addr_un->sun_path, sizeof(musl_addr_un->sun_path), &filc_int_type),
            origin);
        snprintf(addr_un->sun_path, sizeof(addr_un->sun_path), "%s", sun_path);
        filc_deallocate(sun_path);
        *addr = (struct sockaddr*)addr_un;
        *addrlen = sizeof(struct sockaddr_un);
        return true;
    }
    case PF_INET6: {
        if (musl_addrlen < sizeof(struct musl_sockaddr_in6))
            return false;
        struct musl_sockaddr_in6* musl_addr_in6 = (struct musl_sockaddr_in6*)musl_addr;
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)
            filc_allocate_utility(sizeof(struct sockaddr_in6));
        pas_zero_memory(addr_in6, sizeof(struct sockaddr_in6));
        if (verbose)
            pas_log("inet6!\n");
        addr_in6->sin6_family = PF_INET6;
        addr_in6->sin6_port = musl_addr_in6->sin6_port;
        addr_in6->sin6_flowinfo = musl_addr_in6->sin6_flowinfo;
        PAS_ASSERT(sizeof(addr_in6->sin6_addr) == sizeof(musl_addr_in6->sin6_addr));
        memcpy(&addr_in6->sin6_addr, &musl_addr_in6->sin6_addr, sizeof(musl_addr_in6->sin6_addr));
        addr_in6->sin6_scope_id = musl_addr_in6->sin6_scope_id;
        *addr = (struct sockaddr*)addr_in6;
        *addrlen = sizeof(struct sockaddr_in6);
        return true;
    }
    default:
        *addr = NULL;
        *addrlen = 0;
        return false;
    }
}

static void ensure_musl_sockaddr(struct musl_sockaddr** musl_addr, unsigned* musl_addrlen,
                                 unsigned desired_musl_addrlen)
{
    PAS_ASSERT(musl_addr);
    PAS_ASSERT(musl_addrlen);
    PAS_ASSERT(!*musl_addr);
    PAS_ASSERT(!*musl_addrlen);
    
    *musl_addr = filc_allocate_int(desired_musl_addrlen, 1);
    *musl_addrlen = desired_musl_addrlen;
}

static bool to_new_musl_sockaddr(struct sockaddr* addr, unsigned addrlen,
                                 struct musl_sockaddr** musl_addr, unsigned* musl_addrlen)
{
    int musl_family;
    if (!to_musl_domain(addr->sa_family, &musl_family))
        return false;
    switch (addr->sa_family) {
    case PF_INET: {
        PAS_ASSERT(addrlen >= sizeof(struct sockaddr_in));
        ensure_musl_sockaddr(musl_addr, musl_addrlen, sizeof(struct musl_sockaddr_in));
        pas_zero_memory(*musl_addr, sizeof(struct musl_sockaddr_in));
        struct musl_sockaddr_in* musl_addr_in = (struct musl_sockaddr_in*)*musl_addr;
        struct sockaddr_in* addr_in = (struct sockaddr_in*)addr;
        musl_addr_in->sin_family = musl_family;
        musl_addr_in->sin_port = addr_in->sin_port;
        musl_addr_in->sin_addr = addr_in->sin_addr.s_addr;
        return true;
    }
    case PF_LOCAL: {
        PAS_ASSERT(addrlen >= sizeof(struct sockaddr_un));
        ensure_musl_sockaddr(musl_addr, musl_addrlen, sizeof(struct musl_sockaddr_un));
        pas_zero_memory(*musl_addr, sizeof(struct musl_sockaddr_un));
        struct musl_sockaddr_un* musl_addr_un = (struct musl_sockaddr_un*)*musl_addr;
        struct sockaddr_un* addr_un = (struct sockaddr_un*)addr;
        musl_addr_un->sun_family = musl_family;
        PAS_ASSERT(sizeof(addr_un->sun_path) == sizeof(musl_addr_un->sun_path));
        memcpy(musl_addr_un->sun_path, addr_un->sun_path, sizeof(musl_addr_un->sun_path));
        return true;
    }
    case PF_INET6: {
        PAS_ASSERT(addrlen >= sizeof(struct sockaddr_in6));
        ensure_musl_sockaddr(musl_addr, musl_addrlen, sizeof(struct musl_sockaddr_in6));
        pas_zero_memory(*musl_addr, sizeof(struct musl_sockaddr_in6));
        struct musl_sockaddr_in6* musl_addr_in6 = (struct musl_sockaddr_in6*)*musl_addr;
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)addr;
        musl_addr_in6->sin6_family = musl_family;
        musl_addr_in6->sin6_port = addr_in6->sin6_port;
        musl_addr_in6->sin6_flowinfo = addr_in6->sin6_flowinfo;
        PAS_ASSERT(sizeof(addr_in6->sin6_addr) == sizeof(musl_addr_in6->sin6_addr));
        memcpy(&musl_addr_in6->sin6_addr, &addr_in6->sin6_addr, sizeof(musl_addr_in6->sin6_addr));
        musl_addr_in6->sin6_scope_id = addr_in6->sin6_scope_id;
        return true;
    }
    default:
        return false;
    }
}

static bool to_musl_sockaddr(struct sockaddr* addr, unsigned addrlen,
                             struct musl_sockaddr* musl_addr, unsigned* musl_addrlen)
{
    PAS_ASSERT(musl_addrlen);
    
    struct musl_sockaddr* temp_musl_addr = NULL;
    unsigned temp_musl_addrlen = 0;

    if (!to_new_musl_sockaddr(addr, addrlen, &temp_musl_addr, &temp_musl_addrlen)) {
        PAS_ASSERT(!temp_musl_addr);
        PAS_ASSERT(!temp_musl_addrlen);
        return false;
    }

    if (!musl_addr)
        PAS_ASSERT(!*musl_addrlen);

    memcpy(musl_addr, temp_musl_addr, pas_min_uintptr(*musl_addrlen, temp_musl_addrlen));
    *musl_addrlen = temp_musl_addrlen;
    filc_deallocate(temp_musl_addr);
    return true;
}

void pizlonated_f_zsys_bind(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_bind",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    unsigned musl_addrlen = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);
    if (musl_addrlen < sizeof(struct musl_sockaddr))
        goto einval;
    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    struct sockaddr* addr;
    unsigned addrlen;
    if (!from_musl_sockaddr(musl_addr, musl_addrlen, &addr, &addrlen, &origin))
        goto einval;

    if (verbose)
        pas_log("calling bind!\n");
    filc_exit();
    int result = bind(sockfd, addr, addrlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);

    filc_deallocate(addr);
    *(int*)filc_ptr_ptr(rets) = result;
    return;
    
einval:
    set_errno(EINVAL);
    *(int*)filc_ptr_ptr(rets) = -1;
}

static bool from_musl_ai_flags(int musl_flags, int* result)
{
    *result = 0;
    if (check_and_clear(&musl_flags, 0x01))
        *result |= AI_PASSIVE;
    if (check_and_clear(&musl_flags, 0x02))
        *result |= AI_CANONNAME;
    if (check_and_clear(&musl_flags, 0x04))
        *result |= AI_NUMERICHOST;
    if (check_and_clear(&musl_flags, 0x08))
        *result |= AI_V4MAPPED;
    if (check_and_clear(&musl_flags, 0x10))
        *result |= AI_ALL;
    if (check_and_clear(&musl_flags, 0x20))
        *result |= AI_ADDRCONFIG;
    if (check_and_clear(&musl_flags, 0x400))
        *result |= AI_NUMERICSERV;
    return !musl_flags;
}

static bool to_musl_ai_flags(int flags, int* result)
{
    *result = 0;
    if (check_and_clear(&flags, AI_PASSIVE))
        *result |= 0x01;
    if (check_and_clear(&flags, AI_CANONNAME))
        *result |= 0x02;
    if (check_and_clear(&flags, AI_NUMERICHOST))
        *result |= 0x04;
    if (check_and_clear(&flags, AI_V4MAPPED))
        *result |= 0x08;
    if (check_and_clear(&flags, AI_ALL))
        *result |= 0x10;
    if (check_and_clear(&flags, AI_ADDRCONFIG))
        *result |= 0x20;
    if (check_and_clear(&flags, AI_NUMERICSERV))
        *result |= 0x400;
    return !flags;
}

static int to_musl_eai(int eai)
{
    switch (eai) {
    case 0:
        return 0;
    case EAI_BADFLAGS:
        return -1;
    case EAI_NONAME:
        return -2;
    case EAI_AGAIN:
        return -3;
    case EAI_FAIL:
        return -4;
    case EAI_NODATA:
        return -5;
    case EAI_FAMILY:
        return -6;
    case EAI_SOCKTYPE:
        return -7;
    case EAI_SERVICE:
        return -8;
    case EAI_MEMORY:
        return -10;
    case EAI_SYSTEM:
        set_errno(errno);
        return -11;
    case EAI_OVERFLOW:
        return -12;
    default:
        PAS_ASSERT(!"Unexpected error code from getaddrinfo");
        return 0;
    }
}

void pizlonated_f_zsys_getaddrinfo(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getaddrinfo",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr node_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr service_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr hints_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr res_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    struct musl_addrinfo* musl_hints = filc_ptr_ptr(hints_ptr);
    if (musl_hints)
        filc_restrict(hints_ptr, 1, &musl_addrinfo_type, &origin);
    filc_check_access_ptr(res_ptr, &origin);
    struct addrinfo hints;
    pas_zero_memory(&hints, sizeof(hints));
    if (musl_hints) {
        if (!from_musl_ai_flags(musl_hints->ai_flags, &hints.ai_flags)) {
            *(int*)filc_ptr_ptr(rets) = to_musl_eai(EAI_BADFLAGS);
            return;
        }
        if (!from_musl_domain(musl_hints->ai_family, &hints.ai_family)) {
            *(int*)filc_ptr_ptr(rets) = to_musl_eai(EAI_FAMILY);
            return;
        }
        if (!from_musl_socket_type(musl_hints->ai_socktype, &hints.ai_socktype)) {
            *(int*)filc_ptr_ptr(rets) = to_musl_eai(EAI_SOCKTYPE);
            return;
        }
        hints.ai_protocol = musl_hints->ai_protocol;
        if (musl_hints->ai_addrlen
            || filc_ptr_ptr(filc_ptr_load(&musl_hints->ai_addr))
            || filc_ptr_ptr(filc_ptr_load(&musl_hints->ai_canonname))
            || filc_ptr_ptr(filc_ptr_load(&musl_hints->ai_next))) {
            errno = EINVAL;
            *(int*)filc_ptr_ptr(rets) = to_musl_eai(EAI_SYSTEM);
            return;
        }
    }
    const char* node = filc_check_and_get_new_str_or_null(node_ptr, &origin);
    const char* service = filc_check_and_get_new_str_or_null(service_ptr, &origin);
    struct addrinfo* res = NULL;
    int result;
    filc_exit();
    result = getaddrinfo(node, service, &hints, &res);
    filc_enter();
    if (result) {
        *(int*)filc_ptr_ptr(rets) = to_musl_eai(result);
        goto done;
    }
    filc_ptr* addrinfo_ptr = (filc_ptr*)filc_ptr_ptr(res_ptr);
    struct addrinfo* current;
    for (current = res; current; current = current->ai_next) {
        struct musl_addrinfo* musl_current = filc_allocate_one(&musl_addrinfo_heap);
        filc_low_level_ptr_safe_bzero(musl_current, sizeof(struct musl_addrinfo));
        filc_ptr_store(
            addrinfo_ptr,
            filc_ptr_forge_with_size(musl_current, sizeof(struct musl_addrinfo), &musl_addrinfo_type));
        PAS_ASSERT(to_musl_ai_flags(current->ai_flags, &musl_current->ai_flags));
        PAS_ASSERT(to_musl_domain(current->ai_family, &musl_current->ai_family));
        PAS_ASSERT(to_musl_socket_type(current->ai_socktype, &musl_current->ai_socktype));
        musl_current->ai_protocol = current->ai_protocol;
        struct musl_sockaddr* musl_addr = NULL;
        unsigned musl_addrlen = 0;
        PAS_ASSERT(to_new_musl_sockaddr(current->ai_addr, current->ai_addrlen, &musl_addr, &musl_addrlen));
        musl_current->ai_addrlen = musl_addrlen;
        filc_ptr_store(
            &musl_current->ai_addr, filc_ptr_forge_with_size(musl_addr, musl_addrlen, &filc_int_type));
        filc_ptr_store(&musl_current->ai_canonname, filc_strdup(current->ai_canonname));
        addrinfo_ptr = &musl_current->ai_next;
    }
    filc_ptr_store(addrinfo_ptr, filc_ptr_forge_invalid(NULL));
    freeaddrinfo(res);

done:
    filc_deallocate(node);
    filc_deallocate(service);
}

void pizlonated_f_zsys_connect(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_connect",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    unsigned musl_addrlen = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);
    if (musl_addrlen < sizeof(struct musl_sockaddr))
        goto einval;
    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    struct sockaddr* addr;
    unsigned addrlen;
    if (!from_musl_sockaddr(musl_addr, musl_addrlen, &addr, &addrlen, &origin))
        goto einval;

    filc_exit();
    int result = connect(sockfd, addr, addrlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);

    filc_deallocate(addr);
    if (verbose)
        pas_log("connect succeeded!\n");
    *(int*)filc_ptr_ptr(rets) = result;
    return;
    
einval:
    set_errno(EINVAL);
    *(int*)filc_ptr_ptr(rets) = -1;
}

void pizlonated_f_zsys_getsockname(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getsockname",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_addrlen_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), &origin);
    unsigned musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);

    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = (struct sockaddr*)alloca(addrlen);
    filc_exit();
    int result = getsockname(sockfd, addr, &addrlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = result;
        return;
    }
    
    PAS_ASSERT(addrlen <= MAX_SOCKADDRLEN);

    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    /* pass our own copy of musl_addrlen to avoid TOCTOU. */
    PAS_ASSERT(to_musl_sockaddr(addr, addrlen, musl_addr, &musl_addrlen));
    *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    if (verbose)
        pas_log("getsockname succeeded!\n");
    return;
}

void pizlonated_f_zsys_getsockopt(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getsockopt",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    int musl_level = filc_ptr_get_next_int(&args, &origin);
    int musl_optname = filc_ptr_get_next_int(&args, &origin);
    filc_ptr optval_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr optlen_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(optlen_ptr, sizeof(unsigned), &origin);
    unsigned musl_optlen = *(unsigned*)filc_ptr_ptr(optlen_ptr);
    int level;
    if (verbose)
        pas_log("here!\n");
    if (!from_musl_socket_level(musl_level, &level))
        goto einval;
    int optname;
    /* everything is primitive, for now */
    filc_check_access_int(optval_ptr, musl_optlen, &origin);
    void* musl_optval = filc_ptr_ptr(optval_ptr);
    void* optval = musl_optval;
    unsigned optlen = musl_optlen;
    switch (level) {
    case SOL_SOCKET:
        if (!from_musl_so_optname(musl_optname, &optname)) {
            if (verbose)
                pas_log("unrecognized proto\n");
            goto enoprotoopt;
        }
        switch (optname) {
        case SO_SNDBUF:
        case SO_RCVBUF:
        case SO_ERROR:
        case SO_TYPE:
        case SO_KEEPALIVE:
        case SO_REUSEADDR:
        case SO_REUSEPORT:
            break;
        case SO_RCVTIMEO:
            if (musl_optlen < sizeof(struct musl_timeval))
                goto einval;
            optval = alloca(sizeof(struct timeval));
            optlen = sizeof(struct timeval);
            break;
        default:
            if (verbose)
                pas_log("default case proto\n");
            goto enoprotoopt;
        }
        break;
    case IPPROTO_IP:
        if (!from_musl_ip_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case IP_TOS:
        case IP_OPTIONS:
            break;
        default:
            goto enoprotoopt;
        }
        break;
    case IPPROTO_TCP:
        if (!from_musl_tcp_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case TCP_NODELAY:
        case TCP_KEEPALIVE:
        case TCP_KEEPINTVL:
            break;
        default:
            goto enoprotoopt;
        }
        break;
    case IPPROTO_IPV6:
        if (!from_musl_ipv6_optname(musl_optname, &optname))
            goto enoprotoopt;
        switch (optname) {
        case IPV6_V6ONLY:
        case IPV6_TCLASS:
            break;
        default:
            goto enoprotoopt;
        }
        break;
    default:
        goto einval;
    }
    filc_exit();
    int result = getsockopt(sockfd, level, optname, optval, &optlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = result;
        return;
    }
    musl_optlen = optlen;
    switch (level) {
    case SOL_SOCKET:
        switch (optname) {
        case SO_RCVTIMEO: {
            struct timeval* tv = (struct timeval*)optval;
            struct musl_timeval* musl_tv = (struct musl_timeval*)musl_optval;
            musl_tv->tv_sec = tv->tv_sec;
            musl_tv->tv_usec = tv->tv_usec;
            musl_optlen = sizeof(struct musl_timeval);
            break;
        }
        default:
            break;
        }
    default:
        break;
    }
    if (verbose)
        pas_log("all good here!\n");
    *(unsigned*)filc_ptr_ptr(optlen_ptr) = musl_optlen;
    return;

enoprotoopt:
    if (verbose)
        pas_log("bad proto\n");
    set_errno(ENOPROTOOPT);
    *(int*)filc_ptr_ptr(rets) = -1;
    return;
    
einval:
    if (verbose)
        pas_log("einval\n");
    set_errno(EINVAL);
    *(int*)filc_ptr_ptr(rets) = -1;
}

void pizlonated_f_zsys_getpeername(PIZLONATED_SIGNATURE)
{
    static const bool verbose = false;
    
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpeername",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_addrlen_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), &origin);
    unsigned musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);

    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = (struct sockaddr*)alloca(addrlen);
    filc_exit();
    int result = getpeername(sockfd, addr, &addrlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = result;
        return;
    }
    
    PAS_ASSERT(addrlen <= MAX_SOCKADDRLEN);

    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    /* pass our own copy of musl_addrlen to avoid TOCTOU. */
    PAS_ASSERT(to_musl_sockaddr(addr, addrlen, musl_addr, &musl_addrlen));
    *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    if (verbose)
        pas_log("getpeername succeeded!\n");
    return;
}

static bool from_musl_msg_flags(int musl_flags, int* result)
{
    *result = 0;
    if (check_and_clear(&musl_flags, 0x0001))
        *result |= MSG_OOB;
    if (check_and_clear(&musl_flags, 0x0002))
        *result |= MSG_PEEK;
    if (check_and_clear(&musl_flags, 0x0004))
        *result |= MSG_DONTROUTE;
    if (check_and_clear(&musl_flags, 0x0008))
        *result |= MSG_CTRUNC;
    if (check_and_clear(&musl_flags, 0x0020))
        *result |= MSG_TRUNC;
    if (check_and_clear(&musl_flags, 0x0040))
        *result |= MSG_DONTWAIT;
    if (check_and_clear(&musl_flags, 0x0080))
        *result |= MSG_EOR;
    if (check_and_clear(&musl_flags, 0x0100))
        *result |= MSG_WAITALL;
    if (check_and_clear(&musl_flags, 0x4000))
        *result |= MSG_NOSIGNAL;
    return !musl_flags;
}

void pizlonated_f_zsys_sendto(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_sendto",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr buf_ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t len = filc_ptr_get_next_size_t(&args, &origin);
    int musl_flags = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    unsigned musl_addrlen = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(ssize_t), &origin);
    filc_check_access_int(buf_ptr, len, &origin);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);
    int flags;
    if (!from_musl_msg_flags(musl_flags, &flags))
        goto einval;
    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    struct sockaddr* addr;
    unsigned addrlen;
    if (!from_musl_sockaddr(musl_addr, musl_addrlen, &addr, &addrlen, &origin))
        goto einval;
    filc_exit();
    ssize_t result = sendto(sockfd, filc_ptr_ptr(buf_ptr), len, flags, addr, addrlen);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);

    filc_deallocate(addr);
    *(ssize_t*)filc_ptr_ptr(rets) = result;
    return;

einval:
    set_errno(EINVAL);
    *(int*)filc_ptr_ptr(rets) = -1;
}

void pizlonated_f_zsys_recvfrom(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_recvfrom",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr buf_ptr = filc_ptr_get_next_ptr(&args, &origin);
    size_t len = filc_ptr_get_next_size_t(&args, &origin);
    int musl_flags = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_addrlen_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(ssize_t), &origin);
    filc_check_access_int(buf_ptr, len, &origin);
    unsigned musl_addrlen;
    if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), &origin);
        musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
        filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);
    } else
        musl_addrlen = 0;
    int flags;
    if (!from_musl_msg_flags(musl_flags, &flags)) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = NULL;
    if (filc_ptr_ptr(musl_addr_ptr)) {
        addr = (struct sockaddr*)alloca(addrlen);
        pas_zero_memory(addr, addrlen);
    }
    filc_exit();
    ssize_t result = recvfrom(
        sockfd, filc_ptr_ptr(buf_ptr), len, flags,
        addr, filc_ptr_ptr(musl_addrlen_ptr) ? &addrlen : NULL);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    else if (filc_ptr_ptr(musl_addrlen_ptr)) {
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
        /* pass our own copy of musl_addrlen to avoid TOCTOU. */
        PAS_ASSERT(to_musl_sockaddr(addr, addrlen, musl_addr, &musl_addrlen));
        *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    }

    *(ssize_t*)filc_ptr_ptr(rets) = result;
}

struct musl_rlimit {
    unsigned long long rlim_cur;
    unsigned long long rlim_max;
};

static bool from_musl_resource(int musl_resource, int* result)
{
    switch (musl_resource) {
    case 0:
        *result = RLIMIT_CPU;
        return true;
    case 1:
        *result = RLIMIT_FSIZE;
        return true;
    case 2:
        *result = RLIMIT_DATA;
        return true;
    case 3:
        *result = RLIMIT_STACK;
        return true;
    case 4:
        *result = RLIMIT_CORE;
        return true;
    case 5:
        *result = RLIMIT_RSS;
        return true;
    case 6:
        *result = RLIMIT_NPROC;
        return true;
    case 7:
        *result = RLIMIT_NOFILE;
        return true;
    case 8:
        *result = RLIMIT_MEMLOCK;
        return true;
    case 9:
        *result = RLIMIT_AS;
        return true;
    default:
        return false;
    }
}

static unsigned long long to_musl_rlimit_value(rlim_t value)
{
    if (value == RLIM_INFINITY)
        return ~0ULL;
    return value;
}

void pizlonated_f_zsys_getrlimit(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getrlimit",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_resource = filc_ptr_get_next_int(&args, &origin);
    filc_ptr rlim_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(rlim_ptr, sizeof(struct musl_rlimit), &origin);
    int resource;
    if (!from_musl_resource(musl_resource, &resource))
        goto einval;
    struct rlimit rlim;
    filc_exit();
    int result = getrlimit(resource, &rlim);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    else {
        PAS_ASSERT(!result);
        struct musl_rlimit* musl_rlim = (struct musl_rlimit*)filc_ptr_ptr(rlim_ptr);
        musl_rlim->rlim_cur = to_musl_rlimit_value(rlim.rlim_cur);
        musl_rlim->rlim_max = to_musl_rlimit_value(rlim.rlim_max);
    }
    *(int*)filc_ptr_ptr(rets) = result;
    return;

einval:
    set_errno(EINVAL);
    *(int*)filc_ptr_ptr(rets) = -1;
}

void pizlonated_f_zsys_umask(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_umask",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    unsigned mask = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    filc_exit();
    unsigned result = umask(mask);
    filc_enter();
    *(unsigned*)filc_ptr_ptr(rets) = result;
}

struct musl_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

void pizlonated_f_zsys_uname(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_uname",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr buf_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(buf_ptr, sizeof(struct musl_utsname), &origin);
    struct musl_utsname* musl_buf = (struct musl_utsname*)filc_ptr_ptr(buf_ptr);
    struct utsname buf;
    filc_exit();
    PAS_ASSERT(!uname(&buf));
    snprintf(musl_buf->sysname, sizeof(musl_buf->sysname), "%s", buf.sysname);
    snprintf(musl_buf->nodename, sizeof(musl_buf->nodename), "%s", buf.nodename);
    snprintf(musl_buf->release, sizeof(musl_buf->release), "%s", buf.release);
    snprintf(musl_buf->version, sizeof(musl_buf->version), "%s", buf.version);
    snprintf(musl_buf->machine, sizeof(musl_buf->machine), "%s", buf.machine);
    PAS_ASSERT(!getdomainname(musl_buf->domainname, sizeof(musl_buf->domainname) - 1));
    filc_enter();
    musl_buf->domainname[sizeof(musl_buf->domainname) - 1] = 0;
}

struct musl_itimerval {
    struct musl_timeval it_interval;
    struct musl_timeval it_value;
};

void pizlonated_f_zsys_getitimer(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getitimer",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int which = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_value_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_value_ptr, sizeof(struct musl_itimerval), &origin);
    filc_exit();
    struct itimerval value;
    int result = getitimer(which, &value);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    struct musl_itimerval* musl_value = (struct musl_itimerval*)filc_ptr_ptr(musl_value_ptr);
    musl_value->it_interval.tv_sec = value.it_interval.tv_sec;
    musl_value->it_interval.tv_usec = value.it_interval.tv_usec;
    musl_value->it_value.tv_sec = value.it_value.tv_sec;
    musl_value->it_value.tv_usec = value.it_value.tv_usec;
}

void pizlonated_f_zsys_setitimer(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_setitimer",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int which = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_new_value_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_old_value_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(musl_new_value_ptr, sizeof(struct musl_itimerval), &origin);
    if (filc_ptr_ptr(musl_old_value_ptr))
        filc_check_access_int(musl_old_value_ptr, sizeof(struct musl_itimerval), &origin);
    struct itimerval new_value;
    struct musl_itimerval* musl_new_value = (struct musl_itimerval*)filc_ptr_ptr(musl_new_value_ptr);
    new_value.it_interval.tv_sec = musl_new_value->it_interval.tv_sec;
    new_value.it_interval.tv_usec = musl_new_value->it_interval.tv_usec;
    new_value.it_value.tv_sec = musl_new_value->it_value.tv_sec;
    new_value.it_value.tv_usec = musl_new_value->it_value.tv_usec;
    filc_exit();
    struct itimerval old_value;
    int result = setitimer(which, &new_value, &old_value);
    int my_errno = errno;
    filc_enter();
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    struct musl_itimerval* musl_old_value = (struct musl_itimerval*)filc_ptr_ptr(musl_old_value_ptr);
    if (musl_old_value) {
        musl_old_value->it_interval.tv_sec = old_value.it_interval.tv_sec;
        musl_old_value->it_interval.tv_usec = old_value.it_interval.tv_usec;
        musl_old_value->it_value.tv_sec = old_value.it_value.tv_sec;
        musl_old_value->it_value.tv_usec = old_value.it_value.tv_usec;
    }
}

void pizlonated_f_zsys_pause(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_pause",
        .line = 0,
        .column = 0
    };
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int result = pause();
    int my_errno = errno;
    filc_enter();
    PAS_ASSERT(result == -1);
    set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = -1;
}

void pizlonated_f_zsys_pselect(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_pselect",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int nfds = filc_ptr_get_next_int(&args, &origin);
    filc_ptr readfds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr writefds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr exceptfds_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr timeout_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr sigmask_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    PAS_ASSERT(FD_SETSIZE == 1024);
    filc_check_access_int(rets, sizeof(int), &origin);
    FILC_CHECK(
        nfds <= 1024,
        &origin,
        "attempt to select with nfds = %d (should be 1024 or less).",
        nfds);
    if (filc_ptr_ptr(readfds_ptr))
        filc_check_access_int(readfds_ptr, sizeof(fd_set), &origin);
    if (filc_ptr_ptr(writefds_ptr))
        filc_check_access_int(writefds_ptr, sizeof(fd_set), &origin);
    if (filc_ptr_ptr(exceptfds_ptr))
        filc_check_access_int(exceptfds_ptr, sizeof(fd_set), &origin);
    if (filc_ptr_ptr(timeout_ptr))
        filc_check_access_int(timeout_ptr, sizeof(struct musl_timespec), &origin);
    if (filc_ptr_ptr(sigmask_ptr))
        filc_check_access_int(sigmask_ptr, sizeof(struct musl_sigset), &origin);
    fd_set* readfds = (fd_set*)filc_ptr_ptr(readfds_ptr);
    fd_set* writefds = (fd_set*)filc_ptr_ptr(writefds_ptr);
    fd_set* exceptfds = (fd_set*)filc_ptr_ptr(exceptfds_ptr);
    struct musl_timespec* musl_timeout = (struct musl_timespec*)filc_ptr_ptr(timeout_ptr);
    struct timespec timeout;
    if (musl_timeout) {
        timeout.tv_sec = musl_timeout->tv_sec;
        timeout.tv_nsec = musl_timeout->tv_nsec;
    }
    struct musl_sigset* musl_sigmask = (struct musl_sigset*)filc_ptr_ptr(sigmask_ptr);
    sigset_t sigmask;
    if (musl_sigmask)
        from_musl_sigset(musl_sigmask, &sigmask);
    filc_exit();
    int result = pselect(nfds, readfds, writefds, exceptfds,
                         musl_timeout ? &timeout : NULL,
                         musl_sigmask ? &sigmask : NULL);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_getpeereid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpeereid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr uid_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr gid_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(uid_ptr, sizeof(unsigned), &origin);
    filc_check_access_int(gid_ptr, sizeof(unsigned), &origin);
    filc_exit();
    uid_t uid;
    gid_t gid;
    int result = getpeereid(fd, &uid, &gid);
    int my_errno = errno;
    filc_enter();
    PAS_ASSERT(result == -1 || !result);
    if (!result) {
        *(unsigned*)filc_ptr_ptr(uid_ptr) = uid;
        *(unsigned*)filc_ptr_ptr(gid_ptr) = gid;
    } else {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = -1;
    }
}

void pizlonated_f_zsys_kill(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_kill",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int pid = filc_ptr_get_next_int(&args, &origin);
    int musl_sig = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    int sig = from_musl_signum(musl_sig);
    if (sig < 0) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    filc_exit();
    int result = kill(pid, sig);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_raise(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_raise",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_sig = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    int sig = from_musl_signum(musl_sig);
    if (sig < 0) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    filc_exit();
    int result = raise(sig);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_dup(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_dup",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int result = dup(fd);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_dup2(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_dup2",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int oldfd = filc_ptr_get_next_int(&args, &origin);
    int newfd = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int result = dup2(oldfd, newfd);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_sigprocmask(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_sigprocmask",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_how = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_set_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_oldset_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    int how;
    switch (musl_how) {
    case 0:
        how = SIG_BLOCK;
        break;
    case 1:
        how = SIG_UNBLOCK;
        break;
    case 2:
        how = SIG_SETMASK;
        break;
    default:
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    sigset_t* set;
    sigset_t* oldset;
    if (filc_ptr_ptr(musl_set_ptr)) {
        filc_check_access_int(musl_set_ptr, sizeof(struct musl_sigset), &origin);
        set = alloca(sizeof(sigset_t));
        from_musl_sigset((struct musl_sigset*)filc_ptr_ptr(musl_set_ptr), set);
    } else
        set = NULL;
    if (filc_ptr_ptr(musl_oldset_ptr)) {
        filc_check_access_int(musl_oldset_ptr, sizeof(struct musl_sigset), &origin);
        oldset = alloca(sizeof(sigset_t));
        pas_zero_memory(oldset, sizeof(sigset_t));
    } else
        oldset = NULL;
    filc_exit();
    int result = pthread_sigmask(how, set, oldset);
    int my_errno = errno;
    filc_enter();
    PAS_ASSERT(result == -1 || !result);
    if (result < 0) {
        set_errno(my_errno);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    if (filc_ptr_ptr(musl_oldset_ptr)) {
        PAS_ASSERT(oldset);
        to_musl_sigset(oldset, (struct musl_sigset*)filc_ptr_ptr(musl_oldset_ptr));
    }
}

void pizlonated_f_zsys_getpwnam(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_getpwnam",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr name_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    const char* name = filc_check_and_get_new_str(name_ptr, &origin);
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct passwd* passwd = getpwnam(name);
    int my_errno = errno;
    filc_deallocate(name);
    if (!passwd) {
        set_errno(my_errno);
        return;
    }
    struct musl_passwd* musl_passwd = to_musl_passwd_threadlocal(passwd);
    *(filc_ptr*)filc_ptr_ptr(rets) =
        filc_ptr_forge(musl_passwd, musl_passwd, musl_passwd + 1, &musl_passwd_type);
}

void pizlonated_f_zsys_setgroups(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_setgroups",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    size_t size = filc_ptr_get_next_size_t(&args, &origin);
    filc_ptr list_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(unsigned), size, &total_size),
        &origin,
        "size argument too big, causes overflow; size = %zu.",
        size);
    filc_check_access_int(list_ptr, total_size, &origin);
    filc_exit();
    PAS_ASSERT(sizeof(gid_t) == sizeof(unsigned));
    int result = setgroups(size, (gid_t*)filc_ptr_ptr(list_ptr));
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

static void init_dirstream(zdirstream* dirstream, DIR* dir)
{
    /* FIXME: Whenever I add a GC, Ima have to be a hell of a lot more careful about safepoints and
       locking. Probably, it'll mean exiting around lock acquisitions, or at least around the lock
       acquisition slowpath. But who gives a shit for now! */
    pas_lock_lock(&dirstream->lock);
    PAS_ASSERT(!dirstream->dir);
    dirstream->dir = dir;
    dirstream->musl_to_loc_capacity = 10;
    dirstream->musl_to_loc = filc_allocate_utility(sizeof(uint64_t) * dirstream->musl_to_loc_capacity);
    dirstream->musl_to_loc_size = 0;
    pas_lock_unlock(&dirstream->lock);
}

void pizlonated_f_zsys_opendir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_opendir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr name_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    const char* name = filc_check_and_get_new_str(name_ptr, &origin);
    filc_exit();
    DIR* dir = opendir(name);
    int my_errno = errno;
    filc_enter();
    filc_deallocate(name);
    if (!dir) {
        set_errno(my_errno);
        return;
    }
    zdirstream* dirstream = filc_allocate_opaque(&zdirstream_heap);
    init_dirstream(dirstream, dir);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte(dirstream, &zdirstream_type);
}

void pizlonated_f_zsys_fdopendir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fdopendir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int fd = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_exit();
    DIR* dir = fdopendir(fd);
    int my_errno = errno;
    filc_enter();
    if (!dir) {
        set_errno(my_errno);
        return;
    }
    zdirstream* dirstream = filc_allocate_opaque(&zdirstream_heap);
    init_dirstream(dirstream, dir);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte(dirstream, &zdirstream_type);
}

void pizlonated_f_zsys_closedir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_closedir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr dirstream_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_opaque(dirstream_ptr, &zdirstream_type, &origin);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);

    pas_lock_lock(&dirstream->lock);
    FILC_ASSERT(dirstream->dir, &origin);

    filc_exit();
    int result = closedir(dirstream->dir);
    int my_errno = errno;
    filc_enter();

    dirstream->dir = NULL;

    pas_allocation_config allocation_config;
    initialize_utility_allocation_config(&allocation_config);
    filc_deallocate(dirstream->musl_to_loc);
    pas_uint64_hash_map_destruct(&dirstream->loc_to_musl, &allocation_config);
    pas_lock_unlock(&dirstream->lock);

    filc_deallocate(dirstream);

    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

static uint64_t to_musl_dirstream_loc(zdirstream* stream, uint64_t loc)
{
    pas_uint64_hash_map_add_result add_result;
    pas_allocation_config allocation_config;

    PAS_ASSERT(stream->loc_to_musl.table_size == stream->musl_to_loc_size);
        
    initialize_utility_allocation_config(&allocation_config);

    add_result = pas_uint64_hash_map_add(&stream->loc_to_musl, loc, NULL, &allocation_config);
    if (add_result.is_new_entry) {
        PAS_ASSERT(stream->loc_to_musl.table_size == stream->musl_to_loc_size + 1);
        add_result.entry->is_valid = true;
        add_result.entry->key = loc;
        add_result.entry->value = stream->musl_to_loc_size;

        if (stream->musl_to_loc_size >= stream->musl_to_loc_capacity) {
            PAS_ASSERT(stream->musl_to_loc_size == stream->musl_to_loc_capacity);

            size_t new_capacity;
            size_t total_size;
            uint64_t* new_musl_to_loc;

            PAS_ASSERT(!pas_mul_uintptr_overflow(stream->musl_to_loc_capacity, 2, &new_capacity));
            PAS_ASSERT(!pas_mul_uintptr_overflow(new_capacity, sizeof(uint64_t), &total_size));

            new_musl_to_loc = filc_allocate_utility(total_size);
            memcpy(new_musl_to_loc, stream->musl_to_loc, sizeof(uint64_t) * stream->musl_to_loc_size);
            filc_deallocate(stream->musl_to_loc);
            stream->musl_to_loc = new_musl_to_loc;
            stream->musl_to_loc_capacity = new_capacity;
        }
        PAS_ASSERT(stream->musl_to_loc_size < stream->musl_to_loc_capacity);

        stream->musl_to_loc[stream->musl_to_loc_size++] = loc;
    }

    return add_result.entry->value;
}

void pizlonated_f_zsys_readdir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_readdir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr dirstream_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_check_access_opaque(dirstream_ptr, &zdirstream_type, &origin);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);

    pas_lock_lock(&dirstream->lock);
    FILC_ASSERT(dirstream->dir, &origin);

    filc_exit();
    /* FIXME: Maybe we can break readdir in some disastrous way if we reenter via a signal and call it
       again? */
    struct dirent* result = readdir(dirstream->dir);
    int my_errno = errno;
    filc_enter();

    if (result) {
        struct musl_dirent* dirent = &dirstream->dirent;
        dirent->d_ino = result->d_ino;
        dirent->d_reclen = result->d_reclen;
        dirent->d_type = result->d_type; /* Amazingly, these constants line up. */
        snprintf(dirent->d_name, sizeof(dirent->d_name), "%s", result->d_name);
        *(filc_ptr*)filc_ptr_ptr(rets) =
            filc_ptr_forge_with_size(dirent, sizeof(struct musl_dirent), &filc_int_type);
    }
    pas_lock_unlock(&dirstream->lock);
}

void pizlonated_f_zsys_rewinddir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_rewinddir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr dirstream_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_opaque(dirstream_ptr, &zdirstream_type, &origin);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    
    pas_lock_lock(&dirstream->lock);
    FILC_ASSERT(dirstream->dir, &origin);
    filc_exit();
    rewinddir(dirstream->dir);
    filc_enter();
    pas_lock_unlock(&dirstream->lock);
}

void pizlonated_f_zsys_seekdir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_seekdir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr dirstream_ptr = filc_ptr_get_next_ptr(&args, &origin);
    long musl_loc = filc_ptr_get_next_long(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_opaque(dirstream_ptr, &zdirstream_type, &origin);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    
    pas_lock_lock(&dirstream->lock);
    FILC_ASSERT(dirstream->dir, &origin);
    FILC_ASSERT((uint64_t)musl_loc < (uint64_t)dirstream->musl_to_loc_size, &origin);
    long loc = dirstream->musl_to_loc[musl_loc];
    
    filc_exit();
    seekdir(dirstream->dir, loc);
    filc_enter();
    pas_lock_unlock(&dirstream->lock);
}

void pizlonated_f_zsys_telldir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_telldir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr dirstream_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(long), &origin);
    filc_check_access_opaque(dirstream_ptr, &zdirstream_type, &origin);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    
    pas_lock_lock(&dirstream->lock);
    FILC_ASSERT(dirstream->dir, &origin);
    filc_exit();
    long loc = telldir(dirstream->dir);
    int my_errno = errno;
    filc_enter();
    if (loc < 0) {
        pas_lock_unlock(&dirstream->lock);
        set_errno(my_errno);
        *(long*)filc_ptr_ptr(rets) = -1;
        return;
    }
    *(long*)filc_ptr_ptr(rets) = (long)to_musl_dirstream_loc(dirstream, loc);
    pas_lock_unlock(&dirstream->lock);
}

void pizlonated_f_zsys_dirfd(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_dirfd",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr dirstream_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_opaque(dirstream_ptr, &zdirstream_type, &origin);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    
    pas_lock_lock(&dirstream->lock);
    FILC_ASSERT(dirstream->dir, &origin);
    filc_exit();
    int result = dirfd(dirstream->dir);
    int my_errno = errno;
    filc_enter();
    pas_lock_unlock(&dirstream->lock);
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_closelog(PIZLONATED_SIGNATURE)
{
    PIZLONATED_DELETE_ARGS();
    filc_exit();
    closelog();
    filc_enter();
}

void pizlonated_f_zsys_openlog(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_openlog",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr ident_ptr = filc_ptr_get_next_ptr(&args, &origin);
    int option = filc_ptr_get_next_int(&args, &origin);
    int facility = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    const char* ident = filc_check_and_get_new_str(ident_ptr, &origin);
    filc_exit();
    /* Amazingly, the option/facility constants all match up! */
    openlog(ident, option, facility);
    filc_enter();
    filc_deallocate(ident);
}

void pizlonated_f_zsys_setlogmask(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_setlogmask",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int mask = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int result = setlogmask(mask);
    filc_enter();
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_syslog(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_syslog",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int priority = filc_ptr_get_next_int(&args, &origin);
    filc_ptr msg_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    const char* msg = filc_check_and_get_new_str(msg_ptr, &origin);
    filc_exit();
    syslog(priority, "%s", msg);
    filc_enter();
    filc_deallocate(msg);
}

void pizlonated_f_zsys_chdir(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_chdir",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr path_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    const char* path = filc_check_and_get_new_str(path_ptr, &origin);
    filc_exit();
    int result = chdir(path);
    int my_errno = errno;
    filc_enter();
    filc_deallocate(path);
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

static bool have_threads = false;

void pizlonated_f_zsys_fork(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_fork",
        .line = 0,
        .column = 0
    };
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    FILC_ASSERT(!have_threads, &origin);
    pas_scavenger_suspend();
    filc_exit();
    int result = fork();
    int my_errno = errno;
    filc_enter();
    pas_scavenger_resume();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

static int to_musl_wait_status(int status)
{
    if (WIFEXITED(status))
        return WEXITSTATUS(status) << 8;
    if (WIFSIGNALED(status))
        return to_musl_signum(WTERMSIG(status)) | (WCOREDUMP(status) ? 0x80 : 0);
    if (WIFSTOPPED(status))
        return 0x7f | (to_musl_signum(WSTOPSIG(status)) << 8);
    if (WIFCONTINUED(status))
        return 0xffff;
    PAS_ASSERT(!"Should not be reached");
    return 0;
}

void pizlonated_f_zsys_waitpid(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_waitpid",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int pid = filc_ptr_get_next_int(&args, &origin);
    filc_ptr status_ptr = filc_ptr_get_next_ptr(&args, &origin);
    int options = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int status;
    int result = waitpid(pid, &status, options);
    int my_errno = errno;
    filc_enter();
    *(int*)filc_ptr_ptr(rets) = result;
    if (result < 0) {
        PAS_ASSERT(result == -1);
        set_errno(my_errno);
        return;
    }
    if (filc_ptr_ptr(status_ptr)) {
        filc_check_access_int(status_ptr, sizeof(int), &origin);
        *(int*)filc_ptr_ptr(status_ptr) = to_musl_wait_status(status);
    }
}

void pizlonated_f_zsys_listen(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_listen",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    int backlog = filc_ptr_get_next_int(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_exit();
    int result = listen(sockfd, backlog);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_accept(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_accept",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int sockfd = filc_ptr_get_next_int(&args, &origin);
    filc_ptr musl_addr_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr musl_addrlen_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    unsigned musl_addrlen;
    if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), &origin);
        musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
        filc_check_access_int(musl_addr_ptr, musl_addrlen, &origin);
    } else
        musl_addrlen = 0;
    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = NULL;
    if (filc_ptr_ptr(musl_addr_ptr)) {
        addr = (struct sockaddr*)alloca(addrlen);
        pas_zero_memory(addr, addrlen);
    }
    filc_exit();
    int result = accept(sockfd, addr, filc_ptr_ptr(musl_addrlen_ptr) ? &addrlen : NULL);
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    else {
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
        /* pass our own copy of musl_addrlen to avoid TOCTOU. */
        PAS_ASSERT(to_musl_sockaddr(addr, addrlen, musl_addr, &musl_addrlen));
        *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    }
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zsys_socketpair(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zsys_socketpair",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    int musl_domain = filc_ptr_get_next_int(&args, &origin);
    int musl_type = filc_ptr_get_next_int(&args, &origin);
    int protocol = filc_ptr_get_next_int(&args, &origin); /* these constants seem to align between
                                                               Darwin and musl. */
    filc_ptr sv_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(int), &origin);
    filc_check_access_int(sv_ptr, sizeof(int) * 2, &origin);
    int domain;
    if (!from_musl_domain(musl_domain, &domain)) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    int type;
    if (!from_musl_socket_type(musl_type, &type)) {
        set_errno(EINVAL);
        *(int*)filc_ptr_ptr(rets) = -1;
        return;
    }
    filc_exit();
    int result = socketpair(domain, type, protocol, (int*)filc_ptr_ptr(sv_ptr));
    int my_errno = errno;
    filc_enter();
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(rets) = result;
}

void pizlonated_f_zthread_self(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_self",
        .line = 0,
        .column = 0
    };
    filc_ptr rets = PIZLONATED_RETS;
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    zthread* thread = pthread_getspecific(zthread_key);
    PAS_ASSERT(thread);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte(thread, &zthread_type);
}

void pizlonated_f_zthread_get_id(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_get_id",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr thread_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    filc_check_access_opaque(thread_ptr, &zthread_type, &origin);
    zthread* thread = (zthread*)filc_ptr_ptr(thread_ptr);
    *(unsigned*)filc_ptr_ptr(rets) = thread->id;
}

void pizlonated_f_zthread_get_cookie(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_get_cookie",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr thread_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_ptr(rets, &origin);
    filc_check_access_opaque(thread_ptr, &zthread_type, &origin);
    zthread* thread = (zthread*)filc_ptr_ptr(thread_ptr);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_load(&thread->cookie_ptr);
}

void pizlonated_f_zthread_set_self_cookie(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_set_self_cookie",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr cookie_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    zthread* thread = pthread_getspecific(zthread_key);
    PAS_ASSERT(thread);
    filc_ptr_store(&thread->cookie_ptr, cookie_ptr);
}

static void* start_thread(void* arg)
{
    zthread* thread;

    thread = (zthread*)arg;

    PAS_ASSERT(thread->is_running);
    PAS_ASSERT(!pthread_setspecific(zthread_key, thread));

    filc_enter();

    filc_ptr return_buffer;
    pas_zero_memory(&return_buffer, sizeof(return_buffer));

    filc_ptr* args = filc_allocate_one(filc_get_heap(&filc_one_ptr_type));
    filc_ptr_store(args, filc_ptr_load(&thread->arg_ptr));

    PAS_ASSERT(thread->callback);

    thread->callback(args, args + 1, &filc_one_ptr_type,
                     &return_buffer, &return_buffer + 1, &filc_one_ptr_type);

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(thread->is_running);
    filc_ptr_store(&thread->result_ptr, return_buffer);
    pas_lock_unlock(&thread->lock);

    filc_exit();
    
    /* We let zthread_destruct say that the thread is not running. */
    return NULL;
}

void pizlonated_f_zthread_create(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_create",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr callback_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr arg_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    have_threads = true;
    filc_check_access_ptr(rets, &origin);
    filc_check_function_call(callback_ptr, &origin);
    zthread* thread = (zthread*)filc_allocate_opaque(&zthread_heap);
    PAS_ASSERT(thread->id);
    pas_lock_lock(&thread->lock);
    /* I don't see how this could ever happen. */
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(!thread->is_joining);
    PAS_ASSERT(!thread->is_running);
    PAS_ASSERT(should_destroy_thread(thread));
    PAS_ASSERT(filc_ptr_is_totally_null(thread->arg_ptr));
    PAS_ASSERT(filc_ptr_is_totally_null(thread->result_ptr));
    PAS_ASSERT(filc_ptr_is_totally_null(thread->cookie_ptr));
    thread->callback = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(callback_ptr);
    filc_ptr_store(&thread->arg_ptr, arg_ptr);
    thread->is_running = true;
    filc_exit();
    int result = pthread_create(&thread->thread, NULL, start_thread, thread);
    filc_enter();
    if (result) {
        int my_errno = result;
        PAS_ASSERT(!thread->thread);
        thread->is_running = false;
        PAS_ASSERT(should_destroy_thread(thread));
        filc_deallocate(thread);
        pas_lock_unlock(&thread->lock);
        set_errno(my_errno);
        *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_invalid(NULL);
        return;
    }
    PAS_ASSERT(thread->thread);
    PAS_ASSERT(!should_destroy_thread(thread));
    pas_lock_unlock(&thread->lock);
    *(filc_ptr*)filc_ptr_ptr(rets) = filc_ptr_forge_byte(thread, &zthread_type);
}

void pizlonated_f_zthread_join(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_join",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr thread_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr result_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(bool), &origin);
    filc_check_access_opaque(thread_ptr, &zthread_type, &origin);
    if (filc_ptr_ptr(result_ptr))
        filc_check_access_ptr(result_ptr, &origin);
    zthread* thread = (zthread*)filc_ptr_ptr(thread_ptr);
    pas_lock_lock(&thread->lock);
    FILC_CHECK(
        thread->thread,
        &origin,
        "Cannot join thread that has already been joined or detached (thread = %p).",
        thread);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(!thread->is_joining);
    thread->is_joining = true;
    pthread_t system_thread = thread->thread;
    thread->thread = NULL;
    PAS_ASSERT(!should_destroy_thread(thread));
    pas_lock_unlock(&thread->lock);

    filc_exit();
    int result = pthread_join(system_thread, NULL);
    filc_enter();
    if (result) {
        int my_errno = result;
        pas_lock_lock(&thread->lock);
        PAS_ASSERT(!should_destroy_thread(thread));
        PAS_ASSERT(thread->is_joining);
        PAS_ASSERT(!thread->thread);
        thread->is_joining = false;
        thread->thread = system_thread;
        PAS_ASSERT(!should_destroy_thread(thread));
        pas_lock_unlock(&thread->lock);
        set_errno(my_errno);
        *(bool*)filc_ptr_ptr(rets) = false;
        return;
    }

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(thread->is_joining);
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(!thread->is_running);
    thread->is_joining = false;
    if (filc_ptr_ptr(result_ptr))
        filc_ptr_store((filc_ptr*)filc_ptr_ptr(result_ptr), thread->result_ptr);
    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock);
    *(bool*)filc_ptr_ptr(rets) = true;
    return;
}

void pizlonated_f_zthread_detach(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zthread_detach",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr thread_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(bool), &origin);
    filc_check_access_opaque(thread_ptr, &zthread_type, &origin);
    zthread* thread = (zthread*)filc_ptr_ptr(thread_ptr);
    pas_lock_lock(&thread->lock);
    FILC_CHECK(
        thread->thread,
        &origin,
        "Cannot detach thread that has already been joined or detached (thread = %p).",
        thread);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(!thread->is_joining);
    pthread_t system_thread = thread->thread;
    thread->is_joining = true;
    thread->thread = NULL;
    PAS_ASSERT(!should_destroy_thread(thread));
    pas_lock_unlock(&thread->lock);

    filc_exit();
    int result = pthread_detach(system_thread);
    filc_enter();
    if (result) {
        int my_errno = result;
        pas_lock_lock(&thread->lock);
        PAS_ASSERT(!should_destroy_thread(thread));
        PAS_ASSERT(thread->is_joining);
        PAS_ASSERT(!thread->thread);
        thread->is_joining = false;
        thread->thread = system_thread;
        PAS_ASSERT(!should_destroy_thread(thread));
        pas_lock_unlock(&thread->lock);
        set_errno(my_errno);
        *(bool*)filc_ptr_ptr(rets) = false;
        return;
    }

    pas_lock_lock(&thread->lock);
    PAS_ASSERT(!should_destroy_thread(thread));
    PAS_ASSERT(thread->is_joining);
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(!thread->is_running);
    thread->is_joining = false;
    destroy_thread_if_appropriate(thread);
    pas_lock_unlock(&thread->lock);
    *(bool*)filc_ptr_ptr(rets) = true;
    return;
}

typedef struct {
    void (*condition)(PIZLONATED_SIGNATURE);
    void (*before_sleep)(PIZLONATED_SIGNATURE);
    filc_ptr arg_ptr;
} zpark_if_data;

static bool zpark_if_validate_callback(void* arg)
{
    zpark_if_data* data = (zpark_if_data*)arg;

    union {
        uintptr_t return_buffer[2];
        bool result;
    } u;

    pas_zero_memory(u.return_buffer, sizeof(u.return_buffer));

    filc_ptr* args = filc_allocate_one(filc_get_heap(&filc_one_ptr_type));
    filc_ptr_store(args, data->arg_ptr);

    data->condition(args, args + 1, &filc_one_ptr_type,
                    u.return_buffer, u.return_buffer + 2, &filc_int_type);

    return u.result;
}

static void zpark_if_before_sleep_callback(void* arg)
{
    zpark_if_data* data = (zpark_if_data*)arg;

    uintptr_t return_buffer[2];
    pas_zero_memory(return_buffer, sizeof(return_buffer));

    filc_ptr* args = filc_allocate_one(filc_get_heap(&filc_one_ptr_type));
    filc_ptr_store(args, data->arg_ptr);

    data->before_sleep(args, args + 1, &filc_one_ptr_type,
                       return_buffer, return_buffer + 2, &filc_int_type);
}

void pizlonated_f_zpark_if(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zpark_if",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr address_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr condition_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr before_sleep_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr arg_ptr = filc_ptr_get_next_ptr(&args, &origin);
    double absolute_timeout_in_milliseconds = filc_ptr_get_next_double(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(bool), &origin);
    FILC_CHECK(
        filc_ptr_ptr(address_ptr),
        &origin,
        "cannot zpark on a null address.");
    filc_check_function_call(condition_ptr, &origin);
    filc_check_function_call(before_sleep_ptr, &origin);
    zpark_if_data data;
    data.condition = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(condition_ptr);
    data.before_sleep = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(before_sleep_ptr);
    data.arg_ptr = arg_ptr;
    *(bool*)filc_ptr_ptr(rets) = filc_park_conditionally(filc_ptr_ptr(address_ptr),
                                                         zpark_if_validate_callback,
                                                         zpark_if_before_sleep_callback,
                                                         &data,
                                                         absolute_timeout_in_milliseconds);
}

typedef struct {
    void (*callback)(PIZLONATED_SIGNATURE);
    filc_ptr arg_ptr;
} zunpark_one_data;

typedef struct {
    bool did_unpark_thread;
    bool may_have_more_threads;
    filc_ptr arg_ptr;
} zunpark_one_callback_args;

static void zunpark_one_callback(filc_unpark_result result, void* arg)
{
    zunpark_one_data* data = (zunpark_one_data*)arg;

    uintptr_t return_buffer[2];
    pas_zero_memory(return_buffer, sizeof(return_buffer));

    zunpark_one_callback_args* args = filc_allocate_one(filc_get_heap(&filc_int_ptr_type));
    args->did_unpark_thread = result.did_unpark_thread;
    args->may_have_more_threads = result.may_have_more_threads;
    filc_ptr_store(&args->arg_ptr, data->arg_ptr);

    data->callback(args, args + 1, &filc_int_ptr_type,
                   return_buffer, return_buffer + 2, &filc_int_type);
}

void pizlonated_f_zunpark_one(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zunpark_one",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr address_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr callback_ptr = filc_ptr_get_next_ptr(&args, &origin);
    filc_ptr arg_ptr = filc_ptr_get_next_ptr(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    FILC_CHECK(
        filc_ptr_ptr(address_ptr),
        &origin,
        "cannot zpark on a null address.");
    filc_check_function_call(callback_ptr, &origin);
    zunpark_one_data data;
    data.callback = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(callback_ptr);
    data.arg_ptr = arg_ptr;
    filc_unpark_one(filc_ptr_ptr(address_ptr), zunpark_one_callback, &data);
}

void pizlonated_f_zunpark(PIZLONATED_SIGNATURE)
{
    static filc_origin origin = {
        .filename = __FILE__,
        .function = "zunpark",
        .line = 0,
        .column = 0
    };
    filc_ptr args = PIZLONATED_ARGS;
    filc_ptr rets = PIZLONATED_RETS;
    filc_ptr address_ptr = filc_ptr_get_next_ptr(&args, &origin);
    unsigned count = filc_ptr_get_next_unsigned(&args, &origin);
    PIZLONATED_DELETE_ARGS();
    filc_check_access_int(rets, sizeof(unsigned), &origin);
    FILC_CHECK(
        filc_ptr_ptr(address_ptr),
        &origin,
        "cannot zpark on a null address.");
    *(unsigned*)filc_ptr_ptr(rets) = filc_unpark(filc_ptr_ptr(address_ptr), count);
}

#endif /* PAS_ENABLE_FILC */

#endif /* LIBPAS_ENABLED */

