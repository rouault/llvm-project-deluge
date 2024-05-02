#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_runtime.h"

#if PAS_ENABLE_FILC

#include "bmalloc_heap.h"
#include "filc_native.h"
#include "filc_parking_lot.h"
#include "fugc.h"
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
#include "verse_heap_inlines.h"
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
#include <util.h>
#include <grp.h>
#include <utmpx.h>
#include <sys/sysctl.h>
#include <sys/mman.h>
#include <sys/random.h>

#define DEFINE_LOCK(name) \
    pas_system_mutex filc_## name ## _lock; \
    \
    void filc_ ## name ## _lock_lock(void) \
    { \
        pas_system_mutex_lock(&filc_ ## name ## _lock); \
    } \
    \
    void filc_ ## name ## _lock_unlock(void) \
    { \
        pas_system_mutex_unlock(&filc_ ## name ## _lock); \
    } \
    \
    void filc_ ## name ## _lock_assert_held(void) \
    { \
        pas_system_mutex_assert_held(&filc_ ## name ## _lock); \
    } \
    struct filc_dummy

FILC_FOR_EACH_LOCK(DEFINE_LOCK);

PAS_DEFINE_LOCK(filc_soft_handshake);
PAS_DEFINE_LOCK(filc_global_initialization);

filc_thread* filc_first_thread;
pthread_key_t filc_thread_key;
bool filc_is_marking;

pas_heap* filc_default_heap;
pas_heap* filc_destructor_heap;
verse_heap_object_set* filc_destructor_set;

filc_object* filc_free_singleton;

filc_object_array filc_global_variable_roots;

static void* allocate_bmalloc_for_allocation_config(
    size_t size, const char* name, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(name);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    return bmalloc_allocate(size);
}

static void deallocate_bmalloc_for_allocation_config(
    void* ptr, size_t size, pas_allocation_kind allocation_kind, void* arg)
{
    PAS_UNUSED_PARAM(size);
    PAS_ASSERT(allocation_kind == pas_object_allocation);
    PAS_ASSERT(!arg);
    bmalloc_deallocate(ptr);
}

static void initialize_bmalloc_allocation_config(pas_allocation_config* allocation_config)
{
    allocation_config->allocate = allocate_bmalloc_for_allocation_config;
    allocation_config->deallocate = deallocate_bmalloc_for_allocation_config;
    allocation_config->arg = NULL;
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

static void check_musl_passwd(filc_ptr ptr)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_name);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_passwd);
    FILC_CHECK_INT_FIELD(ptr, struct musl_passwd, pw_uid);
    FILC_CHECK_INT_FIELD(ptr, struct musl_passwd, pw_gid);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_gecos);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_dir);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_shell);
}

struct musl_group {
    filc_ptr gr_name;
    filc_ptr gr_passwd;
    unsigned gr_gid;
    filc_ptr gr_mem;
};

static void check_musl_group(filc_ptr ptr)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_group, gr_name);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_group, gr_passwd);
    FILC_CHECK_INT_FIELD(ptr, struct musl_group, gr_gid);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_group, gr_mem);
}

struct musl_sigset {
    unsigned long bits[128 / sizeof(long)];
};

static void check_musl_sigset(filc_ptr ptr)
{
    filc_check_access_int(ptr, sizeof(struct musl_sigset), NULL);
}

struct musl_sigaction {
    filc_ptr sa_handler_ish;
    struct musl_sigset sa_mask;
    int sa_flags;
    filc_ptr sa_restorer; /* ignored */
};

static void check_musl_sigaction(filc_ptr ptr)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_sigaction, sa_handler_ish);
    FILC_CHECK_INT_FIELD(ptr, struct musl_sigaction, sa_mask);
    FILC_CHECK_INT_FIELD(ptr, struct musl_sigaction, sa_flags);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_sigaction, sa_restorer);
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

struct free_tid_node;
typedef struct free_tid_node free_tid_node;
struct free_tid_node {
    unsigned tid;
    free_tid_node* next;
};

static free_tid_node* first_free_tid = NULL;
static unsigned next_fresh_tid = 1;

static unsigned allocate_tid(void)
{
    filc_thread_list_lock_assert_held();
    if (first_free_tid) {
        free_tid_node* result_node = first_free_tid;
        first_free_tid = result_node->next;
        unsigned result = result_node->tid;
        bmalloc_deallocate(result_node);
        return result;
    }
    unsigned result = next_fresh_tid;
    PAS_ASSERT(!pas_add_uint32_overflow(next_fresh_tid, 1, &next_fresh_tid));
    return result;
}

static void deallocate_tid(unsigned tid)
{
    filc_thread_list_lock_assert_held();
    /* FIXME: this is wrong, since it's called in the open. */
    free_tid_node* node = bmalloc_allocate(sizeof(free_tid_node));
    node->tid = tid;
    node->next = first_free_tid;
    first_free_tid = node;
}

filc_thread* filc_thread_create(void)
{
    filc_object* thread_object = filc_allocate_special_early(sizeof(filc_thread), FILC_WORD_TYPE_THREAD);
    filc_thread* thread = thread_object->lower;
    PAS_ASSERT(filc_object_for_special_payload(thread) == thread_object);

    pas_system_mutex_construct(&thread->lock);
    pas_system_condition_construct(&thread->cond);
    filc_object_array_construct(&thread->mark_stack);

    /* The rest of the fields are initialized to zero already. */

    filc_thread_list_lock_lock();
    thread->next_thread = filc_first_thread;
    thread->prev_thread = NULL;
    if (filc_first_thread)
        filc_first_thread->prev_thread = thread;
    filc_first_thread = thread;
    thread->tid = allocate_tid();
    PAS_ASSERT(thread->tid);
    filc_thread_list_lock_unlock();
    
    return thread;
}

void filc_thread_relinquish_tid(filc_thread* thread)
{
    PAS_ASSERT(filc_thread_is_entered(thread));
    filc_thread_list_lock_lock();
    deallocate_tid(thread->tid);
    thread->tid = 0;
    filc_thread_list_lock_unlock();
}

void filc_thread_dispose(filc_thread* thread)
{
    filc_thread_list_lock_lock();
    PAS_ASSERT(!thread->tid);
    if (thread->prev_thread)
        thread->prev_thread->next_thread = thread->next_thread;
    else {
        PAS_ASSERT(filc_first_thread == thread);
        filc_first_thread = thread->next_thread;
    }
    if (thread->next_thread)
        thread->next_thread->prev_thread = thread->prev_thread;
    thread->next_thread = NULL;
    thread->prev_thread = NULL;
    filc_thread_list_lock_unlock();
}

static void check_zthread(filc_ptr ptr)
{
    filc_check_access_special(ptr, FILC_WORD_TYPE_THREAD, NULL);
}

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

static void check_musl_addrinfo(filc_ptr ptr)
{
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_flags);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_family);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_socktype);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_protocol);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_addrlen);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_addrinfo, ai_addr);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_addrinfo, ai_canonname);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_addrinfo, ai_next);
}

struct musl_dirent {
    uint64_t d_ino;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

static void check_musl_dirent(filc_ptr ptr)
{
    filc_check_access_int(ptr, sizeof(struct musl_dirent), NULL);
}

typedef struct {
    pas_lock lock;
    DIR* dir;
    bool in_use;
    uint64_t* musl_to_loc;
    size_t musl_to_loc_size;
    size_t musl_to_loc_capacity;
    pas_uint64_hash_map loc_to_musl;
} zdirstream;

static void zdirstream_check(filc_ptr ptr)
{
    filc_check_access_special(ptr, FILC_WORD_TYPE_DIRSTREAM, NULL);
}

static zdirstream* zdirstream_create(filc_thread* my_thread, DIR* dir)
{
    PAS_ASSERT(my_thread);
    PAS_ASSERT(dir);
    
    filc_object* result_object =
        filc_allocate_special(my_thread, sizeof(zdirstream), FILC_WORD_TYPE_DIRSTREAM);
    zdirstream* result = (zdirstream*)result_object->lower;

    pas_lock_construct(&result->lock);
    result->dir = dir;
    result->in_use = false;
    result->musl_to_loc_capacity = 10;
    result->musl_to_loc = bmalloc_allocate(sizeof(uint64_t) * result->musl_to_loc_capacity);
    result->musl_to_loc_size = 0;
    pas_uint64_hash_map_construct(&result->loc_to_musl);

    return result;
}

struct musl_msghdr {
    filc_ptr msg_name;
    unsigned msg_namelen;
    filc_ptr msg_iov;
    int msg_iovlen;
    filc_ptr msg_control;
    unsigned msg_controllen;
    int msg_flags;
};

static void check_musl_msghdr(filc_ptr ptr)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_msghdr, msg_name);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_namelen);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_msghdr, msg_iov);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_iovlen);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_msghdr, msg_control);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_controllen);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_flags);
}

static filc_signal_handler* signal_table[FILC_MAX_MUSL_SIGNUM + 1];

static bool is_initialized = false; /* Useful for assertions. */
static bool exit_on_panic = false;

void filc_initialize(void)
{
    PAS_ASSERT(!is_initialized);

#define INITIALIZE_LOCK(name) \
    pas_system_mutex_construct(&filc_## name ## _lock)
    FILC_FOR_EACH_LOCK(INITIALIZE_LOCK);
#undef INITIALIZE_LOCK

    filc_default_heap = verse_heap_create(FILC_WORD_SIZE, 0, 0);
    filc_destructor_heap = verse_heap_create(FILC_WORD_SIZE, 0, 0);
    filc_destructor_set = verse_heap_object_set_create();
    verse_heap_add_to_set(filc_destructor_heap, filc_destructor_set);
    verse_heap_did_become_ready_for_allocation();

    filc_free_singleton = (filc_object*)verse_heap_allocate(
        filc_default_heap, pas_round_up_to_power_of_2(PAS_OFFSETOF(filc_object, word_types),
                                                      FILC_WORD_SIZE));
    filc_free_singleton->lower = NULL;
    filc_free_singleton->upper = NULL;
    filc_free_singleton->flags = FILC_OBJECT_FLAG_FREE;

    filc_object_array_construct(&filc_global_variable_roots);

    filc_thread* thread = filc_thread_create();
    thread->has_started = true;
    thread->has_stopped = false;
    thread->thread = pthread_self();
    thread->tlc_node = verse_heap_get_thread_local_cache_node();
    thread->tlc_node_version = pas_thread_local_cache_node_version(thread->tlc_node);
    PAS_ASSERT(!pthread_key_create(&filc_thread_key, NULL));
    PAS_ASSERT(!pthread_setspecific(filc_thread_key, thread));

    /* This has to happen *after* we do our primordial allocations. */
    fugc_initialize();

    exit_on_panic = filc_get_bool_env("FILC_EXIT_ON_PANIC", false);
    
    if (filc_get_bool_env("FILC_DUMP_SETUP", false)) {
        pas_log("filc setup:\n");
        pas_log("    testing library: %s\n", PAS_ENABLE_TESTING ? "yes" : "no");
        pas_log("    exit on panic: %s\n", exit_on_panic ? "yes" : "no");
        fugc_dump_setup();
    }
    
    is_initialized = true;
}

filc_thread* filc_get_my_thread(void)
{
    return (filc_thread*)pthread_getspecific(filc_thread_key);
}

void filc_assert_my_thread_is_not_entered(void)
{
    PAS_ASSERT(!filc_get_my_thread() || !filc_thread_is_entered(filc_get_my_thread()));
}

static void snapshot_threads(filc_thread*** threads, size_t* num_threads)
{
    filc_thread_list_lock_lock();
    *num_threads = 0;
    filc_thread* thread;
    for (thread = filc_first_thread; thread; thread = thread->next_thread) {
        if (thread == filc_first_thread)
            PAS_ASSERT(!thread->prev_thread);
        else
            PAS_ASSERT(thread->prev_thread->next_thread == thread);
        if (thread->next_thread)
            PAS_ASSERT(thread->next_thread->prev_thread == thread);
        (*num_threads)++;
    }
    /* NOTE: This barely works with fork! We snapshot exited, which disagrees with the idea that
       we can only bmalloc_allocate when entered. But, we snapshot when handshaking, an we cannot
       have handshakes in progress at time of fork, so it's fine. */
    *threads = (filc_thread**)bmalloc_allocate(sizeof(filc_thread*) * *num_threads);
    size_t index = 0;
    for (thread = filc_first_thread; thread; thread = thread->next_thread)
        (*threads)[index++] = thread;
    filc_thread_list_lock_unlock();
}

static bool participates_in_handshakes(filc_thread* thread)
{
    return thread->has_started;
}

static bool participates_in_pollchecks(filc_thread* thread)
{
    return participates_in_handshakes(thread) && !thread->is_stopping;
}

static void assert_participates_in_handshakes(filc_thread* thread)
{
    PAS_ASSERT(thread->has_started);
}

static void assert_participates_in_pollchecks(filc_thread* thread)
{
    assert_participates_in_handshakes(thread);
    PAS_ASSERT(!thread->is_stopping);
}

void filc_stop_the_world(void)
{
    static const bool verbose = false;
    
    filc_assert_my_thread_is_not_entered();
    filc_stop_the_world_lock_lock();
    
    sigset_t fullset;
    sigset_t oldset;
    pas_reasonably_fill_sigset(&fullset);
    if (verbose)
        pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &fullset, &oldset));
    
    filc_thread** threads;
    size_t num_threads;
    snapshot_threads(&threads, &num_threads);

    size_t index;
    for (index = num_threads; index--;) {
        filc_thread* thread = threads[index];
        if (!participates_in_handshakes(thread))
            continue;

        pas_system_mutex_lock(&thread->lock);
        for (;;) {
            uint8_t old_state = thread->state;
            PAS_ASSERT(!(old_state & FILC_THREAD_STATE_STOP_REQUESTED));
            uint8_t new_state = old_state | FILC_THREAD_STATE_STOP_REQUESTED;
            if (pas_compare_and_swap_uint8_weak(&thread->state, old_state, new_state))
                break;
        }
        pas_system_mutex_unlock(&thread->lock);
    }

    for (index = num_threads; index--;) {
        filc_thread* thread = threads[index];
        if (!participates_in_handshakes(thread))
            continue;

        pas_system_mutex_lock(&thread->lock);
        while ((thread->state & FILC_THREAD_STATE_ENTERED))
            pas_system_condition_wait(&thread->cond, &thread->lock);
        pas_system_mutex_unlock(&thread->lock);
    }

    bmalloc_deallocate(threads);

    if (verbose)
        pas_log("%s: unblocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
}

void filc_resume_the_world(void)
{
    static const bool verbose = false;
    
    filc_assert_my_thread_is_not_entered();

    sigset_t fullset;
    sigset_t oldset;
    pas_reasonably_fill_sigset(&fullset);
    if (verbose)
        pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &fullset, &oldset));
    
    filc_thread** threads;
    size_t num_threads;
    snapshot_threads(&threads, &num_threads);

    size_t index;
    for (index = num_threads; index--;) {
        filc_thread* thread = threads[index];
        if (!participates_in_handshakes(thread))
            continue;

        pas_system_mutex_lock(&thread->lock);
        for (;;) {
            uint8_t old_state = thread->state;
            PAS_ASSERT(old_state & FILC_THREAD_STATE_STOP_REQUESTED);
            uint8_t new_state = old_state & ~FILC_THREAD_STATE_STOP_REQUESTED;
            if (pas_compare_and_swap_uint8_weak(&thread->state, old_state, new_state))
                break;
        }
        pas_system_condition_broadcast(&thread->cond);
        pas_system_mutex_unlock(&thread->lock);
    }

    bmalloc_deallocate(threads);
    if (verbose)
        pas_log("%s: unblocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
    filc_stop_the_world_lock_unlock();
}

static void run_pollcheck_callback(filc_thread* thread)
{
    /* Worth noting that this may run either with the thread having entered, or with the thread
       having exited. It doesn't matter.
    
       What matters is that we're holding the lock! */
    PAS_ASSERT(thread->state & FILC_THREAD_STATE_CHECK_REQUESTED);
    PAS_ASSERT(thread->pollcheck_callback);
    assert_participates_in_handshakes(thread);
    if (participates_in_pollchecks(thread))
        thread->pollcheck_callback(thread, thread->pollcheck_arg);
    thread->pollcheck_callback = NULL;
    thread->pollcheck_arg = NULL;
    for (;;) {
        uint8_t old_state = thread->state;
        PAS_ASSERT(old_state & FILC_THREAD_STATE_CHECK_REQUESTED);
        uint8_t new_state = old_state & ~FILC_THREAD_STATE_CHECK_REQUESTED;
        if (pas_compare_and_swap_uint8_weak(&thread->state, old_state, new_state))
            break;
    }
    PAS_ASSERT(!(thread->state & FILC_THREAD_STATE_CHECK_REQUESTED));
    PAS_ASSERT(!thread->pollcheck_callback);
    PAS_ASSERT(!thread->pollcheck_arg);
}

/* Returns true if the callback has run already (either because we ran it or because it ran already
   some other way.

   The thread's lock must be held to call this! */
static bool run_pollcheck_callback_from_handshake(filc_thread* thread)
{
    if (!(thread->state & FILC_THREAD_STATE_CHECK_REQUESTED)) {
        PAS_ASSERT(!thread->pollcheck_callback);
        PAS_ASSERT(!thread->pollcheck_arg);
        return true;
    }

    PAS_ASSERT(thread->pollcheck_callback);
    
    if (!(thread->state & FILC_THREAD_STATE_ENTERED)) {
        run_pollcheck_callback(thread);
        return true;
    }

    return false;
}

void filc_soft_handshake_no_op_callback(filc_thread* my_thread, void* arg)
{
    PAS_ASSERT(my_thread);
    PAS_ASSERT(!arg);
}

void filc_soft_handshake(void (*callback)(filc_thread* my_thread, void* arg), void* arg)
{
    static const bool verbose = false;
    
    filc_assert_my_thread_is_not_entered();
    filc_soft_handshake_lock_lock();

    sigset_t fullset;
    sigset_t oldset;
    pas_reasonably_fill_sigset(&fullset);
    if (verbose)
        pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &fullset, &oldset));

    filc_thread** threads;
    size_t num_threads;
    snapshot_threads(&threads, &num_threads);

    /* Tell all the threads that the soft handshake is happening sort of as fast as we possibly
       can, so without calling the callback just yet. We want to maximize the window of time during
       which all threads know that they're supposed to do work for us.
    
       It's questionable if that buys us anything. It does create this kind of situation where
       filc_enter() has to consider the possibility of a pollcheck having been requested, which is
       perhaps awkward. */
    size_t index;
    for (index = num_threads; index--;) {
        filc_thread* thread = threads[index];
        if (!participates_in_handshakes(thread))
            continue;

        pas_system_mutex_lock(&thread->lock);
        PAS_ASSERT(!thread->pollcheck_callback);
        PAS_ASSERT(!thread->pollcheck_arg);
        thread->pollcheck_callback = callback;
        thread->pollcheck_arg = arg;

        for (;;) {
            uint8_t old_state = thread->state;
            PAS_ASSERT(!(old_state & FILC_THREAD_STATE_CHECK_REQUESTED));
            uint8_t new_state = old_state | FILC_THREAD_STATE_CHECK_REQUESTED;
            if (pas_compare_and_swap_uint8_weak(&thread->state, old_state, new_state))
                break;
        }
        pas_system_mutex_unlock(&thread->lock);
    }

    /* Try to run any callbacks we can run ourselves. In the time it takes us to do this, the threads
       that have to run the callbacks themselves might just end up doing it. */
    for (index = num_threads; index--;) {
        filc_thread* thread = threads[index];
        if (!participates_in_handshakes(thread))
            continue;
        
        pas_system_mutex_lock(&thread->lock);
        run_pollcheck_callback_from_handshake(thread);
        pas_system_mutex_unlock(&thread->lock);
    }

    /* Now actually wait for every thread to do it. */
    for (index = num_threads; index--;) {
        filc_thread* thread = threads[index];
        if (!participates_in_handshakes(thread))
            continue;
        
        pas_system_mutex_lock(&thread->lock);
        while (!run_pollcheck_callback_from_handshake(thread))
            pas_system_condition_wait(&thread->cond, &thread->lock);
        pas_system_mutex_unlock(&thread->lock);
    }
    
    bmalloc_deallocate(threads);
    if (verbose)
        pas_log("%s: unblocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
    filc_soft_handshake_lock_unlock();
}

static void run_pollcheck_callback_if_necessary(filc_thread* my_thread)
{
    if (my_thread->state & FILC_THREAD_STATE_CHECK_REQUESTED) {
        run_pollcheck_callback(my_thread);
        pas_system_condition_broadcast(&my_thread->cond);
    }
}

static void stop_if_necessary(filc_thread* my_thread)
{
    while ((my_thread->state & FILC_THREAD_STATE_STOP_REQUESTED)) {
        PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_ENTERED));
        pas_system_condition_wait(&my_thread->cond, &my_thread->lock);
    }
}

void filc_enter(filc_thread* my_thread)
{
    static const bool verbose = false;
    
    /* There's some future world where maybe we turn these into testing asserts. But for now, we only
       enter/exit for syscalls, so it probably doesn't matter and it's probably worth it to do a ton
       of assertions. */
    PAS_ASSERT(my_thread == filc_get_my_thread());
    PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
    PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_ENTERED));
    
    for (;;) {
        uint8_t old_state = my_thread->state;
        PAS_ASSERT(!(old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
        PAS_ASSERT(!(old_state & FILC_THREAD_STATE_ENTERED));
        if ((old_state & (FILC_THREAD_STATE_CHECK_REQUESTED |
                          FILC_THREAD_STATE_STOP_REQUESTED))) {
            /* NOTE: We could avoid doing this if the ENTERED state used by signal handling
               was separate from the ENTERED state used for all other purposes.
            
               Note sure it's worth it, since we would only get here for STOP (super rare) or
               for CHECK requests that happen while we're exited (super rare since that's a
               transient kind of state). */
            sigset_t fullset;
            sigset_t oldset;
            pas_reasonably_fill_sigset(&fullset);
            if (verbose)
                pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
            PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &fullset, &oldset));
            pas_system_mutex_lock(&my_thread->lock);
            PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
            PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_ENTERED));
            run_pollcheck_callback_if_necessary(my_thread);
            while ((my_thread->state & FILC_THREAD_STATE_STOP_REQUESTED)) {
                PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_ENTERED));
                pas_system_condition_wait(&my_thread->cond, &my_thread->lock);
            }
            pas_system_mutex_unlock(&my_thread->lock);
            if (verbose)
                pas_log("%s: unblocking signals\n", __PRETTY_FUNCTION__);
            PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
            continue;
        }

        uint8_t new_state = old_state | FILC_THREAD_STATE_ENTERED;
        if (pas_compare_and_swap_uint8_weak(&my_thread->state, old_state, new_state))
            break;
    }

    PAS_ASSERT((my_thread->state & FILC_THREAD_STATE_ENTERED));
}

static void call_signal_handler(filc_thread* my_thread, filc_signal_handler* handler, int signum)
{
    PAS_ASSERT(handler);
    PAS_ASSERT(handler->musl_signum == signum);

    /* It's likely that we have a top native frame and it's not locked. Lock it to prevent assertions
       in that case. */
    bool was_top_native_frame_unlocked =
        my_thread->top_native_frame && !my_thread->top_native_frame->locked;
    if (was_top_native_frame_unlocked)
        filc_lock_top_native_frame(my_thread);

    static const filc_origin origin = {
        .filename = "<runtime>",
        .function = "call_signal_handler",
        .line = 0,
        .column = 0
    };

    struct {
        FILC_FRAME_BODY;
    } actual_frame;
    pas_zero_memory(&actual_frame, sizeof(actual_frame));
    filc_frame* frame = (filc_frame*)&actual_frame;
    frame->origin = &origin;
    filc_push_frame(my_thread, frame);

    filc_native_frame native_frame;
    filc_push_native_frame(my_thread, &native_frame);

    /* Load the function from the handler first since as soon as we exit, the handler might get GC'd.
       Also, we're choosing not to rely on the fact that functions are global and we track them anyway. */
    filc_ptr function_ptr = filc_ptr_load(my_thread, &handler->function_ptr);

    filc_return_buffer return_buffer;
    filc_ptr rets = filc_ptr_for_int_return_buffer(&return_buffer);
    filc_ptr args = filc_ptr_create(my_thread, filc_allocate_int(my_thread, sizeof(int)));
    *(int*)filc_ptr_ptr(args) = signum;
    /* This check shouldn't be necessary; we do it out of an abundance of paranoia! */
    filc_check_function_call(function_ptr);
    void (*function)(PIZLONATED_SIGNATURE) = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(function_ptr);
    filc_lock_top_native_frame(my_thread);
    function(my_thread, args, rets);
    filc_unlock_top_native_frame(my_thread);

    filc_pop_native_frame(my_thread, &native_frame);
    filc_pop_frame(my_thread, frame);

    if (was_top_native_frame_unlocked)
        filc_unlock_top_native_frame(my_thread);
}

static void handle_deferred_signals(filc_thread* my_thread)
{
    static const bool verbose = false;
    
    PAS_ASSERT(my_thread == filc_get_my_thread());
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    for (;;) {
        uint8_t old_state = my_thread->state;
        if (!(old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL))
            return;
        uint8_t new_state = old_state & ~FILC_THREAD_STATE_DEFERRED_SIGNAL;
        if (pas_compare_and_swap_uint8_weak(&my_thread->state, old_state, new_state))
            break;
    }

    size_t index;
    /* I'm guessing at some point I'll actually have to care about the order here? */
    for (index = FILC_MAX_MUSL_SIGNUM + 1; index--;) {
        uint64_t num_deferred_signals;
        /* We rely on the CAS for a fence, too. */
        for (;;) {
            num_deferred_signals = my_thread->num_deferred_signals[index];
            if (pas_compare_and_swap_uint64_weak(
                    my_thread->num_deferred_signals + index, num_deferred_signals, 0))
                break;
        }
        if (!num_deferred_signals)
            continue;

        if (verbose)
            pas_log("calling signal handler from pollcheck or exit\n");

        /* We're a bit unsafe here because the handler object might get collected at the next exit. */
        filc_signal_handler* handler = signal_table[index];
        PAS_ASSERT(handler);
        sigset_t oldset;
        if (verbose)
            pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
        PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &handler->mask, &oldset));
        while (num_deferred_signals--)
            call_signal_handler(my_thread, handler, (int)index);
        if (verbose)
            pas_log("%s: unblocking signals\n", __PRETTY_FUNCTION__);
        PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
    }
}

void filc_exit(filc_thread* my_thread)
{
    static const bool verbose = false;
    
    PAS_ASSERT(my_thread == filc_get_my_thread());
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    
    for (;;) {
        uint8_t old_state = my_thread->state;
        PAS_ASSERT(old_state & FILC_THREAD_STATE_ENTERED);

        if (old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL) {
            handle_deferred_signals(my_thread);
            continue;
        }

        if ((old_state & FILC_THREAD_STATE_CHECK_REQUESTED)) {
            pas_system_mutex_lock(&my_thread->lock);
            PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
            run_pollcheck_callback_if_necessary(my_thread);
            pas_system_mutex_unlock(&my_thread->lock);
            continue;
        }

        PAS_ASSERT(!(old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
        PAS_ASSERT(!(old_state & FILC_THREAD_STATE_CHECK_REQUESTED));
        uint8_t new_state = old_state & ~FILC_THREAD_STATE_ENTERED;
        if (pas_compare_and_swap_uint8_weak(&my_thread->state, old_state, new_state))
            break;
    }

    PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
    PAS_ASSERT(!(my_thread->state & FILC_THREAD_STATE_ENTERED));

    if ((my_thread->state & FILC_THREAD_STATE_STOP_REQUESTED)) {
        sigset_t fullset;
        sigset_t oldset;
        pas_reasonably_fill_sigset(&fullset);
        if (verbose)
            pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
        PAS_ASSERT(!pthread_sigmask(SIG_BLOCK, &fullset, &oldset));
        pas_system_mutex_lock(&my_thread->lock);
        pas_system_condition_broadcast(&my_thread->cond);
        pas_system_mutex_unlock(&my_thread->lock);
        if (verbose)
            pas_log("%s: unblocking signals\n", __PRETTY_FUNCTION__);
        PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &oldset, NULL));
    }
}

void filc_increase_special_signal_deferral_depth(filc_thread* my_thread)
{
    PAS_ASSERT(my_thread == filc_get_my_thread());
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    my_thread->special_signal_deferral_depth++;
    for (;;) {
        uint8_t old_state = my_thread->state;
        PAS_ASSERT(old_state & FILC_THREAD_STATE_ENTERED);
        if (!(old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL))
            break;
        uint8_t new_state = old_state & ~FILC_THREAD_STATE_DEFERRED_SIGNAL;
        if (pas_compare_and_swap_uint8_weak(&my_thread->state, old_state, new_state)) {
            my_thread->have_deferred_signal_special = true;
            break;
        }
    }
}

void filc_decrease_special_signal_deferral_depth(filc_thread* my_thread)
{
    PAS_ASSERT(my_thread == filc_get_my_thread());
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    PAS_ASSERT(my_thread->special_signal_deferral_depth);
    my_thread->special_signal_deferral_depth--;
    if (!my_thread->special_signal_deferral_depth && my_thread->have_deferred_signal_special) {
        for (;;) {
            uint8_t old_state = my_thread->state;
            PAS_ASSERT(old_state & FILC_THREAD_STATE_ENTERED);
            PAS_ASSERT(!(old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
            uint8_t new_state = old_state | FILC_THREAD_STATE_DEFERRED_SIGNAL;
            if (pas_compare_and_swap_uint8_weak(&my_thread->state, old_state, new_state))
                break;
        }
    }
}

void filc_enter_with_allocation_root(filc_thread* my_thread, filc_object* allocation_root)
{
    filc_enter(my_thread);
    filc_clear_allocation_root(my_thread, allocation_root);
}

void filc_exit_with_allocation_root(filc_thread* my_thread, filc_object* allocation_root)
{
    filc_set_allocation_root(my_thread, allocation_root);
    filc_exit(my_thread);
}

static void enlarge_array(filc_object_array* array, size_t anticipated_size)
{
    PAS_ASSERT(anticipated_size > array->objects_capacity);
    
    filc_object** new_objects;
    size_t new_objects_capacity;
    PAS_ASSERT(!pas_mul_uintptr_overflow(anticipated_size, 3, &new_objects_capacity));
    new_objects_capacity /= 2;
    PAS_ASSERT(new_objects_capacity > array->objects_capacity);
    PAS_ASSERT(new_objects_capacity >= anticipated_size);
    size_t total_size;
    PAS_ASSERT(!pas_mul_uintptr_overflow(new_objects_capacity, sizeof(filc_object*), &total_size));
    new_objects = bmalloc_allocate(total_size);
    memcpy(new_objects, array->objects, array->num_objects * sizeof(filc_object*));
    bmalloc_deallocate(array->objects);
    array->objects = new_objects;
    array->objects_capacity = new_objects_capacity;
}

static void enlarge_array_if_necessary(filc_object_array* array, size_t anticipated_size)
{
    if (anticipated_size > array->objects_capacity)
        enlarge_array(array, anticipated_size);
}

void filc_object_array_push(filc_object_array* array, filc_object* object)
{
    enlarge_array_if_necessary(array, array->num_objects + 1);
    PAS_ASSERT(array->num_objects < array->objects_capacity);
    array->objects[array->num_objects++] = object;
}

void filc_object_array_push_all(filc_object_array* to, filc_object_array* from)
{
    size_t new_num_objects;
    PAS_ASSERT(!pas_add_uintptr_overflow(from->num_objects, to->num_objects, &new_num_objects));
    enlarge_array_if_necessary(to, new_num_objects);
    memcpy(to->objects + to->num_objects, from->objects, sizeof(filc_object*) * from->num_objects);
    to->num_objects += from->num_objects;
}

void filc_object_array_pop_all_from_and_push_to(filc_object_array* from, filc_object_array* to)
{
    if (!from->num_objects)
        return;

    if (!to->num_objects) {
        filc_object_array tmp;
        tmp = *to;
        *to = *from;
        *from = tmp;
        PAS_ASSERT(!from->num_objects);
        return;
    }

    filc_object_array_push_all(to, from);
    filc_object_array_reset(from);
}

void filc_object_array_reset(filc_object_array* array)
{
    filc_object_array_destruct(array);
    filc_object_array_construct(array);
}

void filc_native_frame_add(filc_native_frame* frame, filc_object* object)
{
    PAS_ASSERT(!frame->locked);
    
    if (!object)
        return;

    filc_object_array_push(&frame->array, object);
}

void filc_thread_track_object(filc_thread* my_thread, filc_object* object)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    PAS_TESTING_ASSERT(my_thread->top_native_frame);
    filc_native_frame_add(my_thread->top_native_frame, object);
}

void filc_pollcheck_slow(filc_thread* my_thread, const filc_origin* origin)
{
    PAS_ASSERT(my_thread == filc_get_my_thread());
    PAS_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    if (origin && my_thread->top_frame)
        my_thread->top_frame->origin = origin;

    /* This could be made more efficient, but even if it was, we'd need to have an exit path for the
       STOP_REQUESTED case. */
    filc_exit(my_thread);
    filc_enter(my_thread);
}

void filc_pollcheck_outline(filc_thread* my_thread, const filc_origin* origin)
{
    filc_pollcheck(my_thread, origin);
}

void filc_thread_stop_allocators(filc_thread* my_thread)
{
    assert_participates_in_pollchecks(my_thread);

    pas_thread_local_cache_node* node = my_thread->tlc_node;
    uint64_t version = my_thread->tlc_node_version;
    if (node && version)
        verse_heap_thread_local_cache_node_stop_local_allocators(node, version);
}

void filc_thread_mark_roots(filc_thread* my_thread)
{
    static const bool verbose = false;
    
    assert_participates_in_pollchecks(my_thread);

    filc_object* allocation_root;
    allocation_root = my_thread->allocation_root;
    if (allocation_root) {
        /* Allocation roots have to have the mark bit set without being put on any mark stack, since
           they have no outgoing references and they are not ready for scanning. */
        verse_heap_set_is_marked_relaxed(allocation_root, true);
    }

    filc_frame* frame;
    for (frame = my_thread->top_frame; frame; frame = frame->parent) {
        size_t index;
        for (index = frame->num_objects; index--;) {
            if (verbose)
                pas_log("Marking thread root %p\n", frame->objects[index]);
            fugc_mark(&my_thread->mark_stack, frame->objects[index]);
        }
    }

    filc_native_frame* native_frame;
    for (native_frame = my_thread->top_native_frame; native_frame; native_frame = native_frame->parent) {
        size_t index;
        for (index = native_frame->array.num_objects; index--;)
            fugc_mark(&my_thread->mark_stack, native_frame->array.objects[index]);
    }
}

void filc_thread_sweep_mark_stack(filc_thread* my_thread)
{
    assert_participates_in_pollchecks(my_thread);

    if (my_thread->mark_stack.num_objects) {
        pas_log("Non-empty thread mark stack at start of sweep! Objects:\n");
        size_t index;
        for (index = 0; index < my_thread->mark_stack.num_objects; ++index) {
            filc_object_dump(my_thread->mark_stack.objects[index], &pas_log_stream.base);
            pas_log("\n");
        }
    }
    PAS_ASSERT(!my_thread->mark_stack.num_objects);
    filc_object_array_reset(&my_thread->mark_stack);
}

void filc_thread_donate(filc_thread* my_thread)
{
    assert_participates_in_pollchecks(my_thread);

    fugc_donate(&my_thread->mark_stack);
}

void filc_mark_global_roots(filc_object_array* mark_stack)
{
    size_t index;
    for (index = FILC_MAX_MUSL_SIGNUM + 1; index--;)
        fugc_mark(mark_stack, filc_object_for_special_payload(signal_table[index]));

    fugc_mark(mark_stack, filc_free_singleton);

    filc_global_initialization_lock_lock();
    /* Global roots point to filc_objects that are global, i.e. they are not GC-allocated, but they do
       have outgoing pointers. So, rather than fugc_marking them, we just shove them into the mark
       stack. */
    filc_object_array_push_all(mark_stack, &filc_global_variable_roots);
    filc_global_initialization_lock_unlock();

    filc_thread** threads;
    size_t num_threads;
    snapshot_threads(&threads, &num_threads);
    for (index = num_threads; index--;)
        fugc_mark(mark_stack, filc_object_for_special_payload(threads[index]));
    bmalloc_deallocate(threads);
}

static void signal_pizlonator(int signum)
{
    static const bool verbose = false;
    
    int musl_signum = to_musl_signum(signum);
    PAS_ASSERT((unsigned)musl_signum <= FILC_MAX_MUSL_SIGNUM);
    filc_thread* thread = filc_get_my_thread();

    /* We're running on a thread that shouldn't be receiving signals or we're running in a thread
       that hasn't fully started.
       
       It's possible to handle both cases with enough trickery, but I just assert that it doesn't
       happen, for now.
       
       Quite likely the best kind of trickery here is to send the signal to another thread, unless
       it's a pthread_kill(). We can handle the pthread_kill() case by having pthread_kill() wait
       until the victim thread has properly started and is ready for signals.
    
       Another idea: it sorta seems like starting a thread inherits signals. So, we should start threads
       with all signals blocked, and then problem solved? */
    PAS_ASSERT(thread);
    
    if ((thread->state & FILC_THREAD_STATE_ENTERED) || thread->special_signal_deferral_depth) {
        /* For all we know the user asked for a mask that allows us to recurse, hence the lock-freedom. */
        for (;;) {
            uint64_t old_value = thread->num_deferred_signals[musl_signum];
            if (pas_compare_and_swap_uint64_weak(
                    thread->num_deferred_signals + musl_signum, old_value, old_value + 1))
                break;
        }
        if (thread->special_signal_deferral_depth) {
            thread->have_deferred_signal_special = true;
            return;
        }
        for (;;) {
            uint8_t old_state = thread->state;
            PAS_ASSERT(old_state & FILC_THREAD_STATE_ENTERED);
            if (old_state & FILC_THREAD_STATE_DEFERRED_SIGNAL)
                break;
            uint8_t new_state = old_state | FILC_THREAD_STATE_DEFERRED_SIGNAL;
            if (pas_compare_and_swap_uint8_weak(&thread->state, old_state, new_state))
                break;
        }
        return;
    }

    /* These shenanigans work only because if we ever grab the thread's lock, we are either entered
       (so we won't get here) or we block all signals (so we won't get here). */
    filc_enter(thread);
    /* Even if the signal mask allows the signal to recurse, at this point the signal_pizlonator
       will just count and defer. */

    if (verbose)
        pas_log("calling signal handler from pizlonator\n");
    
    call_signal_handler(thread, signal_table[musl_signum], musl_signum);

    filc_exit(thread);
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

void filc_object_flags_dump_with_comma(filc_object_flags flags, bool* comma, pas_stream* stream)
{
    if (flags & FILC_OBJECT_FLAG_FREE) {
        pas_stream_print_comma(stream, comma, ",");
        pas_stream_printf(stream, "free");
    }
    if (flags & FILC_OBJECT_FLAG_RETURN_BUFFER) {
        pas_stream_print_comma(stream, comma, ",");
        pas_stream_printf(stream, "return_buffer");
    }
    if (flags & FILC_OBJECT_FLAG_SPECIAL) {
        pas_stream_print_comma(stream, comma, ",");
        pas_stream_printf(stream, "special");
    }
    if (flags & FILC_OBJECT_FLAG_GLOBAL) {
        pas_stream_print_comma(stream, comma, ",");
        pas_stream_printf(stream, "global");
    }
    if (flags & FILC_OBJECT_FLAG_MMAP) {
        pas_stream_print_comma(stream, comma, ",");
        pas_stream_printf(stream, "mmap");
    }
    if (flags >> FILC_OBJECT_FLAGS_PIN_SHIFT) {
        pas_stream_print_comma(stream, comma, ",");
        pas_stream_printf(stream, "pinned(%u)", flags >> FILC_OBJECT_FLAGS_PIN_SHIFT);
    }
}

void filc_object_flags_dump(filc_object_flags flags, pas_stream* stream)
{
    if (!flags) {
        pas_stream_printf(stream, "none");
        return;
    }
    bool comma = false;
    filc_object_flags_dump_with_comma(flags, &comma, stream);
}

void filc_object_dump_for_ptr(filc_object* object, void* ptr, pas_stream* stream)
{
    static const bool verbose = false;
    
    if (!object) {
        pas_stream_printf(stream, "<null>");
        return;
    }
    pas_stream_printf(stream, "%p,%p", object->lower, object->upper);
    bool comma = true;
    filc_object_flags_dump_with_comma(object->flags, &comma, stream);
    if (!filc_object_num_words(object)) {
        pas_stream_printf(stream, ",empty");
        return;
    }
    pas_stream_printf(stream, ",");
    size_t start_index;
    size_t end_index;
    size_t highlighted_index;
    bool has_highlighted_index;
    size_t index;
    size_t max_end_index;
    has_highlighted_index = false;
    max_end_index = filc_object_num_words(object) - 1;
    if (ptr < object->lower)
        highlighted_index = 0;
    else if (ptr >= object->upper)
        highlighted_index = max_end_index;
    else {
        highlighted_index = filc_object_word_type_index_for_ptr(object, ptr);
        has_highlighted_index = true;
    }
    PAS_ASSERT(highlighted_index < filc_object_num_words(object));
    PAS_ASSERT(highlighted_index <= max_end_index);
    /* FIXME: We really want a total context length and then if the ptr is on one end, then we pring
       more context on the other end. */
    static const size_t context_radius = 20;
    if (highlighted_index > context_radius)
        start_index = highlighted_index - context_radius;
    else
        start_index = 0;
    if (verbose) {
        pas_log("max_end_index = %zu, highlighted_index = %zu, context_radius = %zu\n",
                max_end_index, highlighted_index, context_radius);
    }
    if (max_end_index - highlighted_index > context_radius)
        end_index = highlighted_index + 1 + context_radius;
    else
        end_index = max_end_index;
    if (verbose) {
        pas_log("start_index = %zu\n", start_index);
        pas_log("end_index = %zu\n", end_index);
    }
    PAS_ASSERT(start_index < filc_object_num_words(object));
    PAS_ASSERT(end_index < filc_object_num_words(object));
    if (start_index)
        pas_stream_printf(stream, "...");
    for (index = start_index; index <= end_index; ++index) {
        if (has_highlighted_index && index == highlighted_index)
            pas_stream_printf(stream, "[");
        filc_word_type_dump(filc_object_get_word_type(object, index), stream);
        if (has_highlighted_index && index == highlighted_index)
            pas_stream_printf(stream, "]");
    }
    if (end_index < max_end_index)
        pas_stream_printf(stream, "...");
}

void filc_object_dump(filc_object* object, pas_stream* stream)
{
    filc_object_dump_for_ptr(object, NULL, stream);
}

void filc_ptr_dump(filc_ptr ptr, pas_stream* stream)
{
    pas_stream_printf(stream, "%p,", filc_ptr_ptr(ptr));
    filc_object_dump_for_ptr(filc_ptr_object(ptr), filc_ptr_ptr(ptr), stream);
}

static char* ptr_to_new_string_impl(filc_ptr ptr, pas_allocation_config* allocation_config)
{
    pas_string_stream stream;
    pas_string_stream_construct(&stream, allocation_config);
    filc_ptr_dump(ptr, &stream.base);
    return pas_string_stream_take_string(&stream);
}

char* filc_object_to_new_string(filc_object* object)
{
    pas_allocation_config allocation_config;
    initialize_bmalloc_allocation_config(&allocation_config);
    pas_string_stream stream;
    pas_string_stream_construct(&stream, &allocation_config);
    filc_object_dump(object, &stream.base);
    return pas_string_stream_take_string(&stream);
}

char* filc_ptr_to_new_string(filc_ptr ptr)
{
    pas_allocation_config allocation_config;
    initialize_bmalloc_allocation_config(&allocation_config);
    return ptr_to_new_string_impl(ptr, &allocation_config);
}

void filc_word_type_dump(filc_word_type type, pas_stream* stream)
{
    switch (type) {
    case FILC_WORD_TYPE_UNSET:
        pas_stream_printf(stream, "_");
        return;
    case FILC_WORD_TYPE_INT:
        pas_stream_printf(stream, "i");
        return;
    case FILC_WORD_TYPE_PTR:
        pas_stream_printf(stream, "P");
        return;
    case FILC_WORD_TYPE_FREE:
        pas_stream_printf(stream, "/");
        return;
    case FILC_WORD_TYPE_FUNCTION:
        pas_stream_printf(stream, "function");
        return;
    case FILC_WORD_TYPE_THREAD:
        pas_stream_printf(stream, "thread");
        return;
    case FILC_WORD_TYPE_DIRSTREAM:
        pas_stream_printf(stream, "dirstream");
        return;
    case FILC_WORD_TYPE_SIGNAL_HANDLER:
        pas_stream_printf(stream, "signal_handler");
        return;
    case FILC_WORD_TYPE_PTR_TABLE:
        pas_stream_printf(stream, "ptr_table");
        return;
    case FILC_WORD_TYPE_PTR_TABLE_ARRAY:
        pas_stream_printf(stream, "ptr_table_array");
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
    initialize_bmalloc_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    filc_word_type_dump(type, &stream.base);
    return pas_string_stream_take_string(&stream);
}

void filc_store_barrier_slow(filc_thread* my_thread, filc_object* object)
{
    PAS_TESTING_ASSERT(!(object->flags & FILC_OBJECT_FLAG_RETURN_BUFFER));
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    fugc_mark(&my_thread->mark_stack, object);
}

void filc_store_barrier_outline(filc_thread* my_thread, filc_object* target)
{
    filc_store_barrier(my_thread, target);
}

static void check_access_common(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin)
{
    if (PAS_ENABLE_TESTING)
        filc_validate_ptr(ptr, origin);

    FILC_CHECK(
        filc_ptr_object(ptr),
        origin,
        "cannot access pointer with null object (ptr = %s).",
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
}

void filc_check_access_special(filc_ptr ptr, filc_word_type word_type, const filc_origin* origin)
{
    PAS_ASSERT(filc_word_type_is_special(word_type));
    
    if (PAS_ENABLE_TESTING)
        filc_validate_ptr(ptr, origin);

    FILC_CHECK(
        filc_ptr_object(ptr),
        origin,
        "cannot access pointer with null object (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    
    FILC_CHECK(
        filc_ptr_ptr(ptr) == filc_ptr_lower(ptr),
        origin,
        "cannot access pointer as %s with ptr != lower (ptr = %s).", 
        filc_word_type_to_new_string(word_type), filc_ptr_to_new_string(ptr));

    FILC_CHECK(
        filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_SPECIAL,
        origin,
        "cannot access pointer as %s, object isn't even special (ptr = %s).",
        filc_word_type_to_new_string(word_type), filc_ptr_to_new_string(ptr));

    FILC_CHECK(
        filc_ptr_object(ptr)->word_types[0] == word_type,
        origin,
        "cannot access pointer as %s, object has wrong special type (ptr = %s).",
        filc_word_type_to_new_string(word_type), filc_ptr_to_new_string(ptr));
}

static void check_not_free(filc_ptr ptr, const filc_origin* origin)
{
    FILC_CHECK(
        !(filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_FREE),
        origin,
        "cannot access pointer to free object (ptr = %s).",
        filc_ptr_to_new_string(ptr));
}

static void check_object_accessible(filc_object* object, const filc_origin* origin)
{
    FILC_CHECK(
        !(object->flags & (FILC_OBJECT_FLAG_FREE | FILC_OBJECT_FLAG_SPECIAL)),
        origin,
        "cannot access pointer to free or special object (object = %s).",
        filc_object_to_new_string(object));
}

static void check_accessible(filc_ptr ptr, const filc_origin* origin)
{
    FILC_CHECK(
        !(filc_ptr_object(ptr)->flags & (FILC_OBJECT_FLAG_FREE | FILC_OBJECT_FLAG_SPECIAL)),
        origin,
        "cannot access pointer to free or special object (ptr = %s).",
        filc_ptr_to_new_string(ptr));
}

filc_ptr filc_get_next_bytes_for_va_arg(
    filc_thread* my_thread, filc_ptr ptr_ptr, size_t size, size_t alignment, const filc_origin* origin)
{
    filc_ptr ptr_value;
    uintptr_t ptr_as_int;
    filc_ptr result;

    filc_check_access_ptr(ptr_ptr, origin);
    filc_ptr* ptr = (filc_ptr*)filc_ptr_ptr(ptr_ptr);

    ptr_value = filc_ptr_load_with_manual_tracking(ptr);
    ptr_as_int = (uintptr_t)filc_ptr_ptr(ptr_value);
    ptr_as_int = pas_round_up_to_power_of_2(ptr_as_int, alignment);

    result = filc_ptr_with_ptr(ptr_value, (void*)ptr_as_int);

    filc_ptr_store(my_thread, ptr, filc_ptr_with_ptr(ptr_value, (char*)ptr_as_int + size));

    return result;
}

filc_object* filc_allocate_special_early(size_t size, filc_word_type word_type)
{
    /* NOTE: This cannot assert anything about the Fil-C thread because we do use this before any
       threads have been created. */

    /* NOTE: This must not exit, because we might hold rando locks while calling into this. */

    PAS_ASSERT(filc_word_type_is_special(word_type));

    pas_heap* heap;
    if (filc_special_word_type_has_destructor(word_type))
        heap = filc_destructor_heap;
    else
        heap = filc_default_heap;

    size_t total_size;
    PAS_ASSERT(!pas_add_uintptr_overflow(FILC_SPECIAL_OBJECT_SIZE, size, &total_size));

    filc_object* result = (filc_object*)verse_heap_allocate(heap, total_size);
    result->lower = (char*)result + FILC_SPECIAL_OBJECT_SIZE;
    result->upper = (char*)result + FILC_SPECIAL_OBJECT_SIZE + FILC_WORD_SIZE;
    result->flags = FILC_OBJECT_FLAG_SPECIAL;
    result->word_types[0] = word_type;
    pas_zero_memory(result->lower, size);
    pas_store_store_fence();

    return result;
}

filc_object* filc_allocate_special(filc_thread* my_thread, size_t size, filc_word_type word_type)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    return filc_allocate_special_early(size, word_type);
}

static void prepare_allocate_object(size_t* size, size_t* num_words, size_t* base_object_size)
{
    size_t original_size = *size;
    *size = pas_round_up_to_power_of_2(*size, FILC_WORD_SIZE);
    PAS_ASSERT(*size >= original_size);
    *num_words = filc_object_num_words_for_size(*size);
    PAS_ASSERT(!pas_add_uintptr_overflow(
                   PAS_OFFSETOF(filc_object, word_types), *num_words, base_object_size));
}

static void initialize_object_with_existing_data(filc_object* result, void* data, size_t size,
                                                 size_t num_words, int8_t object_flags,
                                                 filc_word_type initial_word_type)
{
    result->lower = data;
    result->upper = (char*)data + size;
    result->flags = object_flags;

    size_t index;
    for (index = num_words; index--;)
        result->word_types[index] = initial_word_type;
}

filc_object* filc_allocate_with_existing_data(
    filc_thread* my_thread, void* data, size_t size, int8_t object_flags, filc_word_type initial_word_type)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);
    PAS_ASSERT(!(object_flags & FILC_OBJECT_FLAG_FREE));
    PAS_ASSERT(!(object_flags & FILC_OBJECT_FLAG_RETURN_BUFFER));
    PAS_ASSERT(!(object_flags & FILC_OBJECT_FLAG_SPECIAL));
    PAS_ASSERT(!(object_flags & FILC_OBJECT_FLAG_GLOBAL));

    size_t num_words;
    size_t base_object_size;
    prepare_allocate_object(&size, &num_words, &base_object_size);
    filc_object* result = (filc_object*)verse_heap_allocate(filc_default_heap, base_object_size);
    if (size <= FILC_MAX_BYTES_BETWEEN_POLLCHECKS) {
        initialize_object_with_existing_data(
            result, data, size, num_words, object_flags, initial_word_type);
    } else {
        filc_exit_with_allocation_root(my_thread, result);
        initialize_object_with_existing_data(
            result, data, size, num_words, object_flags, initial_word_type);
        filc_enter_with_allocation_root(my_thread, result);
    }
    return result;
}

static void prepare_allocate(size_t* size, size_t alignment,
                             size_t* num_words, size_t* offset_to_payload, size_t* total_size)
{
    size_t base_object_size;
    prepare_allocate_object(size, num_words, &base_object_size);
    *offset_to_payload = pas_round_up_to_power_of_2(base_object_size, alignment);
    PAS_ASSERT(*offset_to_payload >= base_object_size);
    PAS_ASSERT(!pas_add_uintptr_overflow(*offset_to_payload, *size, total_size));
}

static void initialize_object(filc_object* result, size_t size, size_t num_words,
                              size_t offset_to_payload, filc_word_type initial_word_type)
{
    result->lower = (char*)result + offset_to_payload;
    result->upper = (char*)result + offset_to_payload + size;
    result->flags = 0;

    size_t index;
    for (index = num_words; index--;)
        result->word_types[index] = initial_word_type;
    
    pas_zero_memory((char*)result + offset_to_payload, size);
    
    pas_store_store_fence();
}

static filc_object* finish_allocate(
    filc_thread* my_thread, void* allocation, size_t size, size_t num_words, size_t offset_to_payload,
    filc_word_type initial_word_type)
{
    filc_object* result = (filc_object*)allocation;
    if (size <= FILC_MAX_BYTES_BETWEEN_POLLCHECKS)
        initialize_object(result, size, num_words, offset_to_payload, initial_word_type);
    else {
        filc_exit_with_allocation_root(my_thread, result);
        initialize_object(result, size, num_words, offset_to_payload, initial_word_type);
        filc_enter_with_allocation_root(my_thread, result);
    }
    return result;
}

static filc_object* allocate_impl(filc_thread* my_thread, size_t size, filc_word_type initial_word_type)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    size_t num_words;
    size_t offset_to_payload;
    size_t total_size;
    prepare_allocate(&size, FILC_WORD_SIZE, &num_words, &offset_to_payload, &total_size);
    return finish_allocate(
        my_thread, verse_heap_allocate(filc_default_heap, total_size),
        size, num_words, offset_to_payload, initial_word_type);
}

filc_object* filc_allocate(filc_thread* my_thread, size_t size)
{
    return allocate_impl(my_thread, size, FILC_WORD_TYPE_UNSET);
}

filc_object* filc_allocate_with_alignment(filc_thread* my_thread, size_t size, size_t alignment)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    alignment = pas_max_uintptr(alignment, FILC_WORD_SIZE);
    size_t num_words;
    size_t offset_to_payload;
    size_t total_size;
    prepare_allocate(&size, alignment, &num_words, &offset_to_payload, &total_size);
    return finish_allocate(
        my_thread, verse_heap_allocate_with_alignment(filc_default_heap, total_size, alignment),
        size, num_words, offset_to_payload, FILC_WORD_TYPE_UNSET);
}

filc_object* filc_allocate_int(filc_thread* my_thread, size_t size)
{
    return allocate_impl(my_thread, size, FILC_WORD_TYPE_INT);
}

static filc_object* finish_reallocate(
    filc_thread* my_thread, void* allocation, filc_object* old_object, size_t new_size, size_t num_words,
    size_t offset_to_payload)
{
    static const bool verbose = false;

    if (verbose)
        pas_log("new_size = %zu\n", new_size);
    
    check_object_accessible(old_object, NULL);

    size_t old_num_words = filc_object_num_words(old_object);
    size_t old_size = filc_object_size(old_object);

    size_t common_num_words = pas_min_uintptr(num_words, old_num_words);
    size_t common_size = pas_min_uintptr(new_size, old_size);

    filc_object* result = (filc_object*)allocation;
    filc_set_allocation_root(my_thread, result);
    result->lower = (char*)result + offset_to_payload;
    result->upper = (char*)result + offset_to_payload + new_size;
    result->flags = 0;
    if (new_size > FILC_MAX_BYTES_BETWEEN_POLLCHECKS)
        filc_exit(my_thread);
    size_t index;
    for (index = num_words; index-- > common_num_words;)
        result->word_types[index] = FILC_WORD_TYPE_UNSET;
    pas_zero_memory((char*)result + offset_to_payload + common_size, new_size - common_size);
    if (new_size > FILC_MAX_BYTES_BETWEEN_POLLCHECKS)
        filc_enter(my_thread);
    pas_uint128* dst = (pas_uint128*)((char*)result + offset_to_payload);
    pas_uint128* src = (pas_uint128*)old_object->lower;
    for (index = common_num_words; index--;) {
        for (;;) {
            filc_word_type word_type = old_object->word_types[index];
            /* Don't have to check for freeing here since old_object has to be a malloc object and
               those get freed by GC, so even if a free happened, we still have access to the memory. */
            pas_uint128 word = __c11_atomic_load((_Atomic pas_uint128*)(src + index), __ATOMIC_RELAXED);
            if (word_type == FILC_WORD_TYPE_UNSET) {
                if (word) {
                    /* We have surely raced between someone initializing the word to be not unset, and if
                       we try again we'll see it no longer unset. */
                    pas_fence();
                    continue;
                }
            }
            /* Surely need the barrier since the destination object is black and the source object is
               whatever. */
            if (word_type == FILC_WORD_TYPE_PTR) {
                filc_ptr ptr;
                ptr.word = word;
                filc_store_barrier(my_thread, filc_ptr_object(ptr));
            }
            /* No need for fences or anything like that since the object has not escaped; not even the
               GC can see it. That's because we don't have pollchecks or exits here, which is itself a
               perf bug, see above. */
            result->word_types[index] = word_type;
            __c11_atomic_store((_Atomic pas_uint128*)(dst + index), word, __ATOMIC_RELAXED);
            break;
        }
        filc_pollcheck(my_thread, NULL);
    }

    filc_clear_allocation_root(my_thread, result);
    pas_store_store_fence();
    filc_free(my_thread, old_object);

    return result;
}

filc_object* filc_reallocate(filc_thread* my_thread, filc_object* object, size_t new_size)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    size_t num_words;
    size_t offset_to_payload;
    size_t total_size;
    prepare_allocate(&new_size, FILC_WORD_SIZE, &num_words, &offset_to_payload, &total_size);
    return finish_reallocate(
        my_thread, verse_heap_allocate(filc_default_heap, total_size),
        object, new_size, num_words, offset_to_payload);
}

filc_object* filc_reallocate_with_alignment(filc_thread* my_thread, filc_object* object,
                                            size_t new_size, size_t alignment)
{
    PAS_TESTING_ASSERT(my_thread == filc_get_my_thread());
    PAS_TESTING_ASSERT(my_thread->state & FILC_THREAD_STATE_ENTERED);

    alignment = pas_max_uintptr(alignment, FILC_WORD_SIZE);
    size_t num_words;
    size_t offset_to_payload;
    size_t total_size;
    prepare_allocate(&new_size, FILC_WORD_SIZE, &num_words, &offset_to_payload, &total_size);
    return finish_reallocate(
        my_thread, verse_heap_allocate_with_alignment(filc_default_heap, total_size, alignment),
        object, new_size, num_words, offset_to_payload);
}

void filc_free_yolo(filc_thread* my_thread, filc_object* object)
{
    for (;;) {
        filc_object_flags old_flags = object->flags;
        FILC_CHECK(
            !(old_flags & FILC_OBJECT_FLAG_FREE),
            NULL,
            "cannot free already free object %s.",
            filc_object_to_new_string(object));
        /* Technically, this check is only needed for mmap objects. */
        FILC_CHECK(
            !(old_flags >> FILC_OBJECT_FLAGS_PIN_SHIFT),
            NULL,
            "cannot free pinned object %s.",
            filc_object_to_new_string(object));
        PAS_TESTING_ASSERT(!(old_flags & FILC_OBJECT_FLAG_RETURN_BUFFER));
        filc_object_flags new_flags = old_flags | FILC_OBJECT_FLAG_FREE;
        if (pas_compare_and_swap_uint16_weak_relaxed(&object->flags, old_flags, new_flags))
            break;
    }
    if (filc_object_size(object) > FILC_MAX_BYTES_BETWEEN_POLLCHECKS)
        filc_exit(my_thread);
    size_t index;
    for (index = filc_object_num_words(object); index--;) {
        filc_word_type old_type = filc_object_get_word_type(object, index);
        PAS_TESTING_ASSERT(
            old_type == FILC_WORD_TYPE_UNSET ||
            old_type == FILC_WORD_TYPE_INT ||
            old_type == FILC_WORD_TYPE_PTR ||
            old_type == FILC_WORD_TYPE_DIRSTREAM);
        /* If this was a ptr, and now it's not, then this would be like overwriting a pointer, from
           the GC's standpoint. It's a pointer deletion. But we don't have a deletion barrier! So
           it's fine! */
        object->word_types[index] = FILC_WORD_TYPE_FREE;
    }
    if (filc_object_size(object) > FILC_MAX_BYTES_BETWEEN_POLLCHECKS)
        filc_enter(my_thread);
}

void filc_free(filc_thread* my_thread, filc_object* object)
{
    FILC_CHECK(
        !(object->flags & FILC_OBJECT_FLAG_SPECIAL),
        NULL,
        "cannot free special object %s.",
        filc_object_to_new_string(object));
    FILC_CHECK(
        !(object->flags & FILC_OBJECT_FLAG_GLOBAL),
        NULL,
        "cannot free global object %s.",
        filc_object_to_new_string(object));
    FILC_CHECK(
        !(object->flags & FILC_OBJECT_FLAG_MMAP),
        NULL,
        "cannot free mmap object %s.",
        filc_object_to_new_string(object));
    filc_free_yolo(my_thread, object);
}

static size_t num_ptrtables = 0;

filc_ptr_table* filc_ptr_table_create(filc_thread* my_thread)
{
    filc_ptr_table* result = (filc_ptr_table*)
        filc_allocate_special(my_thread, sizeof(filc_ptr_table), FILC_WORD_TYPE_PTR_TABLE)->lower;

    pas_lock_construct(&result->lock);
    filc_ptr_uintptr_hash_map_construct(&result->encode_map);
    result->free_indices_capacity = 10;
    result->free_indices = bmalloc_allocate(sizeof(uintptr_t) * result->free_indices_capacity);
    result->num_free_indices = 0;
    result->array = filc_ptr_table_array_create(my_thread, 10);

    if (PAS_ENABLE_TESTING)
        pas_atomic_exchange_add_uintptr(&num_ptrtables, 1);

    return result;
}

void filc_ptr_table_destruct(filc_ptr_table* ptr_table)
{
    static const bool verbose = false;
    if (verbose)
        pas_log("Destructing ptrtable\n");
    pas_allocation_config allocation_config;
    initialize_bmalloc_allocation_config(&allocation_config);
    filc_ptr_uintptr_hash_map_destruct(&ptr_table->encode_map, &allocation_config);
    bmalloc_deallocate(ptr_table->free_indices);

    if (PAS_ENABLE_TESTING)
        pas_atomic_exchange_add_uintptr(&num_ptrtables, (intptr_t)-1);
}

static uintptr_t ptr_table_encode_holding_lock(
    filc_thread* my_thread, filc_ptr_table* ptr_table, filc_ptr ptr)
{
    pas_allocation_config allocation_config;
    initialize_bmalloc_allocation_config(&allocation_config);

    PAS_ASSERT(ptr_table->array);

    /* This function does the store barrier twice, but like, whatever. */
    filc_store_barrier(my_thread, filc_ptr_object(ptr));
    filc_ptr_uintptr_hash_map_add_result add_result =
        filc_ptr_uintptr_hash_map_add(&ptr_table->encode_map, ptr, NULL, &allocation_config);
    if (!add_result.is_new_entry) {
        uintptr_t result = add_result.entry->value;
        PAS_ASSERT(result < ptr_table->array->num_entries);
        PAS_ASSERT(result < ptr_table->array->capacity);
        return (result + FILC_PTR_TABLE_OFFSET) << FILC_PTR_TABLE_SHIFT;
    }

    uintptr_t result;
    if (ptr_table->num_free_indices)
        result = ptr_table->free_indices[--ptr_table->num_free_indices];
    else {
        if (ptr_table->array->num_entries >= ptr_table->array->capacity) {
            PAS_ASSERT(ptr_table->array->num_entries == ptr_table->array->capacity);
            size_t new_capacity = ptr_table->array->capacity << 1;
            PAS_ASSERT(new_capacity > ptr_table->array->capacity);
            filc_ptr_table_array* new_array = filc_ptr_table_array_create(my_thread, new_capacity);

            /* There's some universe where we do this loop exited, but it probably just doesn't matter
               at all. */
            size_t index;
            for (index = ptr_table->array->num_entries; index--;) {
                filc_ptr_store(
                    my_thread, new_array->ptrs + index,
                    filc_ptr_load_with_manual_tracking(ptr_table->array->ptrs + index));
            }

            new_array->num_entries = ptr_table->array->num_entries;
            ptr_table->array = new_array;
        }

        PAS_ASSERT(ptr_table->array->num_entries < ptr_table->array->capacity);
        result = ptr_table->array->num_entries++;
    }

    PAS_ASSERT(result < ptr_table->array->num_entries);
    PAS_ASSERT(result < ptr_table->array->capacity);
    filc_ptr_store(my_thread, ptr_table->array->ptrs + result, ptr);
    return (result + FILC_PTR_TABLE_OFFSET) << FILC_PTR_TABLE_SHIFT;
}

uintptr_t filc_ptr_table_encode(filc_thread* my_thread, filc_ptr_table* ptr_table, filc_ptr ptr)
{
    if (!filc_ptr_ptr(ptr) || !filc_ptr_object(ptr)
        || (filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_FREE))
        return 0;
    pas_lock_lock(&ptr_table->lock);
    uintptr_t result = ptr_table_encode_holding_lock(my_thread, ptr_table, ptr);
    pas_lock_unlock(&ptr_table->lock);
    return result;
}

filc_ptr filc_ptr_table_decode_with_manual_tracking(filc_ptr_table* ptr_table, uintptr_t encoded_ptr)
{
    filc_ptr_table_array* array = ptr_table->array;
    
    size_t index = (encoded_ptr >> FILC_PTR_TABLE_SHIFT) - FILC_PTR_TABLE_OFFSET;
    if (index >= array->num_entries)
        return filc_ptr_forge_null();

    /* NULL shouldn't have gotten this far. */
    PAS_TESTING_ASSERT(encoded_ptr);

    filc_ptr result = filc_ptr_load_with_manual_tracking(array->ptrs + index);
    if (!filc_ptr_ptr(result))
        return filc_ptr_forge_null();

    PAS_TESTING_ASSERT(filc_ptr_object(result));
    if (filc_ptr_object(result)->flags & FILC_OBJECT_FLAG_FREE)
        return filc_ptr_forge_null();

    return result;
}

void filc_ptr_table_mark_outgoing_ptrs(filc_ptr_table* ptr_table, filc_object_array* stack)
{
    static const bool verbose = false;
    if (verbose)
        pas_log("Marking ptr table at %p.\n", ptr_table);
    /* This needs to rehash the the whole table, marking non-free objects, and just skipping the free
       ones.
       
       Then it needs to walk the array and remove the free entries, putting their indices into the
       free_indices array.
    
       This may result in the hashtable and the array disagreeing a bit, and that's fine. They'll only
       disagree on things that are free.
    
       If the hashtable has an entry that the array doesn't have: this means that the object in question
       is free, so we'll never look up that entry in the hashtable due to the free check. New objects
       that take the same address will get a fresh entry in the hashtable and a fresh index.
    
       If the array has an entry that the hashtable doesn't have: decoding that object will fail the
       free check, so you won't be able to tell that the object has an index. Adding new objects that
       take the same address won't be able to reuse that index, because it'll seem to be taken. */

    pas_lock_lock(&ptr_table->lock);

    pas_allocation_config allocation_config;
    initialize_bmalloc_allocation_config(&allocation_config);

    filc_ptr_uintptr_hash_map new_encode_map;
    filc_ptr_uintptr_hash_map_construct(&new_encode_map);
    size_t index;
    for (index = ptr_table->encode_map.table_size; index--;) {
        filc_ptr_uintptr_hash_map_entry entry = ptr_table->encode_map.table[index];
        if (filc_ptr_uintptr_hash_map_entry_is_empty_or_deleted(entry))
            continue;
        if (filc_ptr_object(entry.key)->flags & FILC_OBJECT_FLAG_FREE)
            continue;
        fugc_mark(stack, filc_ptr_object(entry.key));
        filc_ptr_uintptr_hash_map_add_new(&new_encode_map, entry, NULL, &allocation_config);
    }
    filc_ptr_uintptr_hash_map_destruct(&ptr_table->encode_map, &allocation_config);
    ptr_table->encode_map = new_encode_map;

    fugc_mark(stack, filc_object_for_special_payload(ptr_table->array));

    /* It's not necessary to mark entries in this array, since they'll be marked when we
       filc_ptr_table_array_mark_outgoing_ptrs(). It's not clear that we could avoid marking them in
       that function, though maybe we could avoid it. */
    for (index = ptr_table->array->num_entries; index--;) {
        filc_ptr ptr = filc_ptr_load_with_manual_tracking(ptr_table->array->ptrs + index);
        if (!filc_ptr_ptr(ptr))
            continue;
        if (!(filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_FREE))
            continue;
        if (ptr_table->num_free_indices >= ptr_table->free_indices_capacity) {
            PAS_ASSERT(ptr_table->num_free_indices == ptr_table->free_indices_capacity);

            size_t new_free_indices_capacity = ptr_table->free_indices_capacity << 1;
            PAS_ASSERT(new_free_indices_capacity > ptr_table->free_indices_capacity);

            uintptr_t* new_free_indices = bmalloc_allocate(sizeof(uintptr_t) * new_free_indices_capacity);
            memcpy(new_free_indices, ptr_table->free_indices,
                   sizeof(uintptr_t) * ptr_table->num_free_indices);

            bmalloc_deallocate(ptr_table->free_indices);
            ptr_table->free_indices = new_free_indices;
            ptr_table->free_indices_capacity = new_free_indices_capacity;
        }
        PAS_ASSERT(ptr_table->num_free_indices < ptr_table->free_indices_capacity);
        ptr_table->free_indices[ptr_table->num_free_indices++] = index;
        filc_ptr_store_without_barrier(ptr_table->array->ptrs + index, filc_ptr_forge_null());
    }
    
    pas_lock_unlock(&ptr_table->lock);
}

filc_ptr_table_array* filc_ptr_table_array_create(filc_thread* my_thread, size_t capacity)
{
    size_t array_size;
    PAS_ASSERT(!pas_mul_uintptr_overflow(sizeof(filc_ptr), capacity, &array_size));
    size_t total_size;
    PAS_ASSERT(!pas_add_uintptr_overflow(
                   PAS_OFFSETOF(filc_ptr_table_array, ptrs), array_size, &total_size));
    
    filc_ptr_table_array* result = (filc_ptr_table_array*)
        filc_allocate_special(my_thread, total_size, FILC_WORD_TYPE_PTR_TABLE_ARRAY)->lower;
    result->capacity = capacity;

    return result;
}

void filc_ptr_table_array_mark_outgoing_ptrs(filc_ptr_table_array* array, filc_object_array* stack)
{
    size_t index;
    for (index = array->num_entries; index--;)
        fugc_mark(stack, filc_ptr_object(filc_ptr_load_with_manual_tracking(array->ptrs + index)));
}

void filc_pin(filc_object* object)
{
    if (!object)
        return;
    if (object->flags & FILC_OBJECT_FLAG_GLOBAL)
        return;
    for (;;) {
        filc_object_flags old_flags = object->flags;
        FILC_CHECK(
            !(old_flags & FILC_OBJECT_FLAG_FREE),
            NULL,
            "cannot pin free object %s.",
            filc_object_to_new_string(object));
        PAS_ASSERT(!(old_flags & FILC_OBJECT_FLAG_RETURN_BUFFER));
        filc_object_flags new_flags = old_flags + ((filc_object_flags)1 << FILC_OBJECT_FLAGS_PIN_SHIFT);
        FILC_CHECK(
            new_flags >> FILC_OBJECT_FLAGS_PIN_SHIFT,
            NULL,
            "pin count overflow in %s.",
            filc_object_to_new_string(object));
        if (pas_compare_and_swap_uint16_weak_relaxed(&object->flags, old_flags, new_flags))
            break;
    }
}

void filc_unpin(filc_object* object)
{
    if (!object)
        return;
    if (object->flags & FILC_OBJECT_FLAG_GLOBAL)
        return;
    for (;;) {
        filc_object_flags old_flags = object->flags;
        PAS_ASSERT(!(old_flags & FILC_OBJECT_FLAG_FREE)); /* should never happen */
        PAS_ASSERT(old_flags >> FILC_OBJECT_FLAGS_PIN_SHIFT);
        PAS_ASSERT(!(old_flags & FILC_OBJECT_FLAG_RETURN_BUFFER));
        PAS_ASSERT(old_flags >= ((filc_object_flags)1 << FILC_OBJECT_FLAGS_PIN_SHIFT));
        filc_object_flags new_flags = old_flags - ((filc_object_flags)1 << FILC_OBJECT_FLAGS_PIN_SHIFT);
        if (pas_compare_and_swap_uint16_weak_relaxed(&object->flags, old_flags, new_flags))
            break;
    }
}

filc_ptr filc_native_zalloc(filc_thread* my_thread, size_t size)
{
    return filc_ptr_create_with_manual_tracking(filc_allocate(my_thread, size));
}

filc_ptr filc_native_zaligned_alloc(filc_thread* my_thread, size_t alignment, size_t size)
{
    return filc_ptr_create_with_manual_tracking(filc_allocate_with_alignment(my_thread, size, alignment));
}

static filc_object* object_for_deallocate(filc_ptr ptr)
{
    FILC_CHECK(
        filc_ptr_object(ptr),
        NULL,
        "cannot free ptr with no object (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    FILC_CHECK(
        filc_ptr_ptr(ptr) == filc_ptr_lower(ptr),
        NULL,
        "cannot free ptr with ptr != lower (ptr = %s).",
        filc_ptr_to_new_string(ptr));
    return filc_ptr_object(ptr);
}

filc_ptr filc_native_zrealloc(filc_thread* my_thread, filc_ptr old_ptr, size_t size)
{
    static const bool verbose = false;
    
    if (!filc_ptr_ptr(old_ptr))
        return filc_native_zalloc(my_thread, size);
    if (verbose)
        pas_log("zrealloc to size = %zu\n", size);
    return filc_ptr_create_with_manual_tracking(
        filc_reallocate(my_thread, object_for_deallocate(old_ptr), size));
}

filc_ptr filc_native_zaligned_realloc(filc_thread* my_thread,
                                      filc_ptr old_ptr, size_t alignment, size_t size)
{
    if (!filc_ptr_ptr(old_ptr))
        return filc_native_zaligned_alloc(my_thread, alignment, size);
    return filc_ptr_create_with_manual_tracking(
        filc_reallocate_with_alignment(my_thread, object_for_deallocate(old_ptr), alignment, size));
}

void filc_native_zfree(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    if (!filc_ptr_ptr(ptr))
        return;
    filc_free(my_thread, object_for_deallocate(ptr));
}

filc_ptr filc_native_zgetlower(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    return filc_ptr_with_ptr(ptr, filc_ptr_lower(ptr));
}

filc_ptr filc_native_zgetupper(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    return filc_ptr_with_ptr(ptr, filc_ptr_upper(ptr));
}

bool filc_native_zisunset(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    check_access_common(ptr, 1, NULL);
    check_not_free(ptr, NULL);
    if (filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_SPECIAL)
        return false;
    uintptr_t offset = filc_ptr_offset(ptr);
    uintptr_t word_type_index = offset / FILC_WORD_SIZE;
    return filc_object_get_word_type(filc_ptr_object(ptr), word_type_index) == FILC_WORD_TYPE_UNSET;
}

bool filc_native_zisint(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    check_access_common(ptr, 1, NULL);
    check_not_free(ptr, NULL);
    if (filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_SPECIAL)
        return false;
    uintptr_t offset = filc_ptr_offset(ptr);
    uintptr_t word_type_index = offset / FILC_WORD_SIZE;
    return filc_object_get_word_type(filc_ptr_object(ptr), word_type_index) == FILC_WORD_TYPE_INT;
}

int filc_native_zptrphase(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    check_access_common(ptr, 1, NULL);
    check_not_free(ptr, NULL);
    if (filc_ptr_object(ptr)->flags & FILC_OBJECT_FLAG_SPECIAL)
        return -1;
    uintptr_t offset = filc_ptr_offset(ptr);
    uintptr_t word_type_index = offset / FILC_WORD_SIZE;
    uintptr_t offset_in_word = offset % FILC_WORD_SIZE;
    if (filc_object_get_word_type(filc_ptr_object(ptr), word_type_index) != FILC_WORD_TYPE_PTR)
        return -1;
    return (int)offset_in_word;
}

filc_ptr filc_native_zptrtable_new(filc_thread* my_thread)
{
    return filc_ptr_for_special_payload_with_manual_tracking(filc_ptr_table_create(my_thread));
}

size_t filc_native_zptrtable_encode(filc_thread* my_thread, filc_ptr table_ptr, filc_ptr ptr)
{
    filc_check_access_special(table_ptr, FILC_WORD_TYPE_PTR_TABLE, NULL);
    return filc_ptr_table_encode(my_thread, (filc_ptr_table*)filc_ptr_ptr(table_ptr), ptr);
}

filc_ptr filc_native_zptrtable_decode(filc_thread* my_thread, filc_ptr table_ptr, size_t encoded_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    filc_check_access_special(table_ptr, FILC_WORD_TYPE_PTR_TABLE, NULL);
    return filc_ptr_table_decode_with_manual_tracking(
        (filc_ptr_table*)filc_ptr_ptr(table_ptr), encoded_ptr);
}

size_t filc_native_ztesting_get_num_ptrtables(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    return num_ptrtables;
}

void filc_validate_object(filc_object* object, const filc_origin* origin)
{
    if (object == filc_free_singleton) {
        FILC_ASSERT(!object->lower, origin);
        FILC_ASSERT(!object->upper, origin);
        FILC_ASSERT(object->flags == FILC_OBJECT_FLAG_FREE, origin);
        return;
    }

    FILC_ASSERT(object->lower, origin);
    FILC_ASSERT(object->upper, origin);
    
    if (object->flags & FILC_OBJECT_FLAG_SPECIAL) {
        FILC_ASSERT(object->upper == (char*)object->lower + FILC_WORD_SIZE, origin);
        FILC_ASSERT(object->word_types[0] == FILC_WORD_TYPE_FREE ||
                    object->word_types[0] == FILC_WORD_TYPE_FUNCTION ||
                    object->word_types[0] == FILC_WORD_TYPE_THREAD ||
                    object->word_types[0] == FILC_WORD_TYPE_DIRSTREAM ||
                    object->word_types[0] == FILC_WORD_TYPE_SIGNAL_HANDLER ||
                    object->word_types[0] == FILC_WORD_TYPE_PTR_TABLE ||
                    object->word_types[0] == FILC_WORD_TYPE_PTR_TABLE_ARRAY,
                    origin);
        if (object->word_types[0] != FILC_WORD_TYPE_FUNCTION)
            FILC_ASSERT(pas_is_aligned((uintptr_t)object->lower, FILC_WORD_SIZE), origin);
        return;
    }

    FILC_ASSERT(pas_is_aligned((uintptr_t)object->lower, FILC_WORD_SIZE), origin);
    FILC_ASSERT(pas_is_aligned((uintptr_t)object->upper, FILC_WORD_SIZE), origin);
    FILC_ASSERT(object->upper >= object->lower, origin);

    size_t index;
    for (index = filc_object_num_words(object); index--;) {
        filc_word_type word_type = filc_object_get_word_type(object, index);
        FILC_ASSERT(word_type == FILC_WORD_TYPE_UNSET ||
                    word_type == FILC_WORD_TYPE_INT ||
                    word_type == FILC_WORD_TYPE_PTR ||
                    word_type == FILC_WORD_TYPE_FREE,
                    origin);
    }
}

void filc_validate_normal_object(filc_object* object, const filc_origin* origin)
{
    FILC_ASSERT(!(object->flags & FILC_OBJECT_FLAG_RETURN_BUFFER), origin);
    filc_validate_object(object, origin);
}

void filc_validate_return_buffer_object(filc_object* object, const filc_origin* origin)
{
    FILC_ASSERT(object->flags & FILC_OBJECT_FLAG_RETURN_BUFFER, origin);
    FILC_ASSERT(!(object->flags & FILC_OBJECT_FLAG_SPECIAL), origin);
    FILC_ASSERT(!(object->flags & FILC_OBJECT_FLAG_FREE), origin);
    filc_validate_object(object, origin);
}

void filc_validate_ptr(filc_ptr ptr, const filc_origin* origin)
{
    if (filc_ptr_is_boxed_int(ptr))
        return;

    filc_validate_object(filc_ptr_object(ptr), origin);
}

void filc_validate_normal_ptr(filc_ptr ptr, const filc_origin* origin)
{
    if (filc_ptr_is_boxed_int(ptr))
        return;

    filc_validate_normal_object(filc_ptr_object(ptr), origin);
}

void filc_validate_return_buffer_ptr(filc_ptr ptr, const filc_origin* origin)
{
    FILC_ASSERT(!filc_ptr_is_boxed_int(ptr), origin);
    filc_validate_return_buffer_object(filc_ptr_object(ptr), origin);
}

filc_ptr filc_native_zptr_to_new_string(filc_thread* my_thread, filc_ptr ptr)
{
    char* str = filc_ptr_to_new_string(ptr);
    filc_ptr result = filc_strdup(my_thread, str);
    bmalloc_deallocate(str);
    return result;
}

static void check_int(filc_ptr ptr, uintptr_t bytes, const filc_origin* origin)
{
    uintptr_t offset;
    uintptr_t first_word_type_index;
    uintptr_t last_word_type_index;
    uintptr_t word_type_index;

    offset = filc_ptr_offset(ptr);
    first_word_type_index = offset / FILC_WORD_SIZE;
    last_word_type_index = (offset + bytes - 1) / FILC_WORD_SIZE;

    /* FIXME: Eventually, we'll want this to exit.
     
       If we do make it exit, then we'll have to make sure that we check that the object is not
       FREE, since any exit might observe munmap. */

    for (word_type_index = first_word_type_index;
         word_type_index <= last_word_type_index;
         word_type_index++) {
        for (;;) {
            filc_word_type word_type = filc_object_get_word_type(filc_ptr_object(ptr), word_type_index);
            if (word_type == FILC_WORD_TYPE_UNSET) {
                if (pas_compare_and_swap_uint8_weak(
                        filc_ptr_object(ptr)->word_types + word_type_index,
                        FILC_WORD_TYPE_UNSET,
                        FILC_WORD_TYPE_INT))
                    break;
                continue;
            }
            
            FILC_CHECK(
                word_type == FILC_WORD_TYPE_INT,
                origin,
                "cannot access %zu bytes as int, span contains non-ints (ptr = %s).",
                bytes, filc_ptr_to_new_string(ptr));
            break;
        }
    }
}

void filc_check_access_int(filc_ptr ptr, size_t bytes, const filc_origin* origin)
{
    if (!bytes)
        return;
    check_access_common(ptr, bytes, origin);
    check_int(ptr, bytes, origin);
}

void filc_check_access_ptr(filc_ptr ptr, const filc_origin* origin)
{
    uintptr_t offset;
    uintptr_t word_type_index;

    check_access_common(ptr, sizeof(filc_ptr), origin);

    offset = filc_ptr_offset(ptr);
    FILC_CHECK(
        pas_is_aligned(offset, FILC_WORD_SIZE),
        origin,
        "cannot access memory as ptr without 16-byte alignment; in this case ptr %% 16 = %zu (ptr = %s).",
        (size_t)(offset % FILC_WORD_SIZE), filc_ptr_to_new_string(ptr));
    word_type_index = offset / FILC_WORD_SIZE;
    
    for (;;) {
        filc_word_type word_type = filc_object_get_word_type(filc_ptr_object(ptr), word_type_index);
        if (word_type == FILC_WORD_TYPE_UNSET) {
            if (pas_compare_and_swap_uint8_weak(
                    filc_ptr_object(ptr)->word_types + word_type_index,
                    FILC_WORD_TYPE_UNSET,
                    FILC_WORD_TYPE_PTR))
                break;
            continue;
        }
        
        FILC_CHECK(
            word_type == FILC_WORD_TYPE_PTR,
            origin,
            "cannot access %zu bytes as ptr, word is non-ptr (ptr = %s).",
            FILC_WORD_SIZE, filc_ptr_to_new_string(ptr));
        break;
    }
}

void filc_check_function_call(filc_ptr ptr)
{
    filc_check_access_special(ptr, FILC_WORD_TYPE_FUNCTION, NULL);
}

void filc_memset_with_exit(
    filc_thread* my_thread, filc_object* object, void* ptr, unsigned value, size_t bytes)
{
    if (bytes <= FILC_MAX_BYTES_BETWEEN_POLLCHECKS) {
        memset(ptr, value, bytes);
        return;
    }
    filc_pin(object);
    filc_exit(my_thread);
    memset(ptr, value, bytes);
    filc_enter(my_thread);
    filc_unpin(object);
}

void filc_memcpy_with_exit(
    filc_thread* my_thread, filc_object* dst_object, filc_object* src_object,
    void* dst, const void* src, size_t bytes)
{
    if (bytes <= FILC_MAX_BYTES_BETWEEN_POLLCHECKS) {
        memcpy(dst, src, bytes);
        return;
    }
    filc_pin(dst_object);
    filc_pin(src_object);
    filc_exit(my_thread);
    memcpy(dst, src, bytes);
    filc_enter(my_thread);
    filc_unpin(dst_object);
    filc_unpin(src_object);
}

void filc_memmove_with_exit(
    filc_thread* my_thread, filc_object* dst_object, filc_object* src_object,
    void* dst, const void* src, size_t bytes)
{
    if (bytes <= FILC_MAX_BYTES_BETWEEN_POLLCHECKS) {
        memmove(dst, src, bytes);
        return;
    }
    filc_pin(dst_object);
    filc_pin(src_object);
    filc_exit(my_thread);
    memmove(dst, src, bytes);
    filc_enter(my_thread);
    filc_unpin(dst_object);
    filc_unpin(src_object);
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

void filc_low_level_ptr_safe_bzero_with_exit(
    filc_thread* my_thread, filc_object* object, void* raw_ptr, size_t bytes)
{
    if (bytes <= FILC_MAX_BYTES_BETWEEN_POLLCHECKS) {
        filc_low_level_ptr_safe_bzero(raw_ptr, bytes);
        return;
    }
    filc_pin(object);
    filc_exit(my_thread);
    filc_low_level_ptr_safe_bzero(raw_ptr, bytes);
    filc_enter(my_thread);
    filc_unpin(object);
}

void filc_memset(filc_thread* my_thread, filc_ptr ptr, unsigned value, size_t count,
                 const filc_origin* origin)
{
    static const bool verbose = false;
    char* raw_ptr;
    
    if (!count)
        return;

    raw_ptr = filc_ptr_ptr(ptr);

    if (verbose)
        pas_log("count = %zu\n", count);
    check_access_common(ptr, count, origin);
    
    if (!value) {
        /* FIXME: If the hanging chads in this range are already UNSET, then we don't have to do
           anything. In particular, we could leave them UNSET and then skip the memset.
           
           But, we cnanot leave them UNSET and do the memset since that might race with someone
           converting the range to PTR and result in a partially-nulled ptr. */
        
        char* start = raw_ptr;
        char* end = raw_ptr + count;
        char* aligned_start = (char*)pas_round_up_to_power_of_2((uintptr_t)start, FILC_WORD_SIZE);
        char* aligned_end = (char*)pas_round_down_to_power_of_2((uintptr_t)end, FILC_WORD_SIZE);
        if (aligned_start > end || aligned_end < start) {
            check_int(ptr, count, origin);
            memset(raw_ptr, 0, count);
            return;
        }
        if (aligned_start > start) {
            check_int(ptr, aligned_start - start, origin);
            memset(start, 0, aligned_start - start);
        }
        check_accessible(ptr, origin);
        filc_low_level_ptr_safe_bzero_with_exit(
            my_thread, filc_ptr_object(ptr), aligned_start, aligned_end - aligned_start);
        if (end > aligned_end) {
            check_int(filc_ptr_with_ptr(ptr, aligned_end), end - aligned_end, origin);
            memset(aligned_end, 0, end - aligned_end);
        }
        return;
    }

    check_int(ptr, count, origin);
    filc_memset_with_exit(my_thread, filc_ptr_object(ptr), raw_ptr, value, count);
}

void filc_memmove(filc_thread* my_thread, filc_ptr dst, filc_ptr src, size_t count,
                  const filc_origin* origin)
{
    static const bool verbose = false;
    
    char* raw_dst_ptr;
    char* raw_src_ptr;
    
    if (!count)
        return;

    check_access_common(dst, count, origin);
    check_access_common(src, count, origin);

    raw_dst_ptr = filc_ptr_ptr(dst);
    raw_src_ptr = filc_ptr_ptr(src);

    if (pas_is_aligned((uintptr_t)raw_dst_ptr, FILC_WORD_SIZE) &&
        pas_is_aligned(count, FILC_WORD_SIZE)) {

        if (verbose)
            pas_log("Taking ptr-safe memmove path.\n");

        if (origin)
            my_thread->top_frame->origin = origin;

        static const filc_origin copy_origin = {
            .function = "filc_memmove",
            .filename = "<native>",
            .line = 0,
            .column = 0
        };
        struct {
            FILC_FRAME_BODY;
            filc_object* objects[2];
        } actual_frame;
        pas_zero_memory(&actual_frame, sizeof(actual_frame));
        filc_frame* copy_frame = (filc_frame*)&actual_frame;
        copy_frame->origin = &copy_origin;
        copy_frame->num_objects = 2;
        copy_frame->objects[0] = filc_ptr_object(dst);
        copy_frame->objects[1] = filc_ptr_object(src);
        filc_push_frame(my_thread, copy_frame);
        
        bool src_can_has_ptrs = pas_is_aligned((uintptr_t)raw_src_ptr, FILC_WORD_SIZE);
        
        check_accessible(dst, NULL);
        if (src_can_has_ptrs)
            check_accessible(src, NULL);
        else
            check_int(src, count, NULL);

        pas_uint128* cur_dst = (pas_uint128*)raw_dst_ptr;
        pas_uint128* cur_src = (pas_uint128*)raw_src_ptr;
        filc_object* dst_object = filc_ptr_object(dst);
        filc_object* src_object = filc_ptr_object(src);
        size_t cur_dst_word_index = (raw_dst_ptr - (char*)dst_object->lower) / FILC_WORD_SIZE;
        size_t cur_src_word_index = (raw_src_ptr - (char*)src_object->lower) / FILC_WORD_SIZE;
        size_t countdown = count / FILC_WORD_SIZE;

        bool is_up = true;
        if (cur_dst > cur_src) {
            is_up = false;
            cur_dst += countdown - 1;
            cur_src += countdown - 1;
            cur_dst_word_index += countdown - 1;
            cur_src_word_index += countdown - 1;
        }

        while (countdown--) {
            for (;;) {
                filc_word_type src_word_type;
                pas_uint128 src_word;
                if (src_can_has_ptrs) {
                    src_word_type = filc_object_get_word_type(src_object, cur_src_word_index);
                    src_word = __c11_atomic_load((_Atomic pas_uint128*)cur_src, __ATOMIC_RELAXED);
                } else {
                    src_word_type = FILC_WORD_TYPE_INT;
                    src_word = *cur_src;
                }
                if (!src_word) {
                    /* copying an unset zero word is always legal to any destination type, no
                       problem. it's even OK to copy a zero into free memory. and there's zero value
                       in changing the destination type from unset to anything. */
                    __c11_atomic_store((_Atomic pas_uint128*)cur_dst, 0, __ATOMIC_RELAXED);
                    break;
                }
                if (src_word_type == FILC_WORD_TYPE_UNSET) {
                    /* We have surely raced between someone initializing the word to be not unset, and if
                       we try again we'll see it no longer unset. */
                    pas_fence();
                    continue;
                }
                FILC_CHECK(
                    src_word_type == FILC_WORD_TYPE_INT ||
                    src_word_type == FILC_WORD_TYPE_PTR,
                    NULL,
                    "cannot copy anything but int or ptr (dst = %s, src = %s).",
                    filc_ptr_to_new_string(filc_ptr_with_ptr(dst, cur_dst)),
                    filc_ptr_to_new_string(filc_ptr_with_ptr(src, cur_src)));
                filc_word_type dst_word_type = filc_object_get_word_type(dst_object, cur_dst_word_index);
                if (dst_word_type == FILC_WORD_TYPE_UNSET) {
                    if (!pas_compare_and_swap_uint8_weak(
                            dst_object->word_types + cur_dst_word_index,
                            FILC_WORD_TYPE_UNSET,
                            src_word_type))
                        continue;
                } else {
                    FILC_CHECK(
                        src_word_type == dst_word_type,
                        NULL,
                        "type mismatch while copying (dst = %s, src = %s).",
                        filc_ptr_to_new_string(filc_ptr_with_ptr(dst, cur_dst)),
                        filc_ptr_to_new_string(filc_ptr_with_ptr(src, cur_src)));
                }
                if (src_word_type == FILC_WORD_TYPE_PTR) {
                    filc_ptr ptr;
                    ptr.word = src_word;
                    filc_store_barrier(my_thread, filc_ptr_object(ptr));
                }
                __c11_atomic_store((_Atomic pas_uint128*)cur_dst, src_word, __ATOMIC_RELAXED);
                break;
            }
            if (is_up) {
                cur_dst++;
                cur_src++;
                cur_dst_word_index++;
                cur_src_word_index++;
            } else {
                cur_dst--;
                cur_src--;
                cur_dst_word_index--;
                cur_src_word_index--;
            }
            if (filc_pollcheck(my_thread, NULL)) {
                check_accessible(dst, NULL);
                check_accessible(src, NULL);
            }
        }
        
        filc_pop_frame(my_thread, copy_frame);
        return;
    }

    if (verbose)
        pas_log("Taking int-only memmove path.\n");
    check_int(dst, count, origin);
    check_int(src, count, origin);
    filc_memmove_with_exit(
        my_thread, filc_ptr_object(dst), filc_ptr_object(src), raw_dst_ptr, raw_src_ptr, count);
}

static char* finish_check_and_get_new_str(char* base, size_t length)
{
    char* result = bmalloc_allocate(length + 1);
    memcpy(result, base, length + 1);
    FILC_ASSERT(!result[length], NULL);
    return result;
}

char* filc_check_and_get_new_str(filc_ptr str)
{
    size_t available;
    size_t length;
    check_access_common(str, 1, NULL);
    available = filc_ptr_available(str);
    length = strnlen((char*)filc_ptr_ptr(str), available);
    FILC_ASSERT(length < available, NULL);
    FILC_ASSERT(length + 1 <= available, NULL);
    check_int(str, length + 1, NULL);

    return finish_check_and_get_new_str((char*)filc_ptr_ptr(str), length);
}

char* filc_check_and_get_new_str_for_int_memory(char* base, size_t size)
{
    size_t length;
    FILC_ASSERT(size, NULL);
    length = strnlen(base, size);
    FILC_ASSERT(length < size, NULL);
    FILC_ASSERT(length + 1 <= size, NULL);

    return finish_check_and_get_new_str(base, length);
}

char* filc_check_and_get_new_str_or_null(filc_ptr ptr)
{
    if (filc_ptr_ptr(ptr))
        return filc_check_and_get_new_str(ptr);
    return NULL;
}

filc_ptr filc_strdup(filc_thread* my_thread, const char* str)
{
    if (!str)
        return filc_ptr_forge_null();
    size_t size = strlen(str) + 1;
    filc_ptr result = filc_ptr_create(my_thread, filc_allocate_int(my_thread, size));
    filc_memcpy_with_exit(my_thread, filc_ptr_object(result), NULL, filc_ptr_ptr(result), str, size);
    return result;
}

filc_global_initialization_context* filc_global_initialization_context_create(
    filc_global_initialization_context* parent)
{
    static const bool verbose = false;
    
    filc_global_initialization_context* result;

    if (verbose)
        pas_log("creating context with parent = %p\n", parent);
    
    if (parent) {
        parent->ref_count++;
        return parent;
    }

    /* Can't exit to grab this lock, because the GC might grab it, and we support running the GC in
       STW mode.
       
       Also, not need to exit to grab this lock, since we don't exit while the lock is held anyway. */
    filc_global_initialization_lock_lock();
    result = (filc_global_initialization_context*)
        bmalloc_allocate(sizeof(filc_global_initialization_context));
    if (verbose)
        pas_log("new context at %p\n", result);
    result->ref_count = 1;
    pas_ptr_hash_map_construct(&result->map);

    return result;
}

bool filc_global_initialization_context_add(
    filc_global_initialization_context* context, filc_ptr* pizlonated_gptr, filc_object* object)
{
    static const bool verbose = false;
    
    pas_allocation_config allocation_config;

    filc_global_initialization_lock_assert_held();
    filc_testing_validate_object(object, NULL);
    PAS_ASSERT(object->flags & FILC_OBJECT_FLAG_GLOBAL);

    if (verbose)
        pas_log("dealing with pizlonated_gptr = %p\n", pizlonated_gptr);

    filc_ptr gptr_value = filc_ptr_load_with_manual_tracking(pizlonated_gptr);
    if (filc_ptr_ptr(gptr_value)) {
        PAS_ASSERT(filc_ptr_lower(gptr_value) == filc_ptr_ptr(gptr_value));
        PAS_ASSERT(filc_ptr_object(gptr_value) == object);
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

    initialize_bmalloc_allocation_config(&allocation_config);

    if (verbose)
        pas_log("object = %s\n", filc_object_to_new_string(object));

    pas_ptr_hash_map_add_result add_result = pas_ptr_hash_map_add(
        &context->map, pizlonated_gptr, NULL, &allocation_config);
    if (!add_result.is_new_entry) {
        if (verbose)
            pas_log("was already seen\n");
        filc_object* existing_object = add_result.entry->value;
        PAS_ASSERT(existing_object == object);
        return false;
    }

    if (verbose)
        pas_log("going to initialize object = %s\n", filc_object_to_new_string(object));

    filc_object_array_push(&filc_global_variable_roots, object);

    add_result.entry->key = pizlonated_gptr;
    add_result.entry->value = object;

    return true;
}

void filc_global_initialization_context_destroy(filc_global_initialization_context* context)
{
    static const bool verbose = false;
    
    size_t index;
    pas_allocation_config allocation_config;

    PAS_ASSERT(context->ref_count);
    if (--context->ref_count)
        return;

    if (verbose)
        pas_log("destroying/comitting context at %p\n", context);
    
    pas_store_store_fence();

    for (index = context->map.table_size; index--;) {
        pas_ptr_hash_map_entry entry = context->map.table[index];
        if (pas_ptr_hash_map_entry_is_empty_or_deleted(entry))
            continue;
        filc_ptr* pizlonated_gptr = (filc_ptr*)entry.key;
        filc_object* object = (filc_object*)entry.value;
        PAS_TESTING_ASSERT(filc_ptr_is_totally_null(*pizlonated_gptr));
        filc_testing_validate_object(object, NULL);
        filc_ptr_store_without_barrier(pizlonated_gptr, filc_ptr_create_with_manual_tracking(object));
    }

    initialize_bmalloc_allocation_config(&allocation_config);

    pas_ptr_hash_map_destruct(&context->map, &allocation_config);
    bmalloc_deallocate(context);
    filc_global_initialization_lock_unlock();
}

static filc_ptr get_constant_value(filc_constant_kind kind, void* target,
                                   filc_global_initialization_context* context)
{
    switch (kind) {
    case filc_global_constant: {
        filc_ptr result = ((filc_ptr (*)(filc_global_initialization_context*))target)(context);
        PAS_ASSERT(filc_ptr_object(result));
        PAS_ASSERT(filc_ptr_ptr(result));
        return result;
    }
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
    return filc_ptr_forge_null();
}

void filc_execute_constant_relocations(
    void* constant, filc_constant_relocation* relocations, size_t num_relocations,
    filc_global_initialization_context* context)
{
    static const bool verbose = false;
    size_t index;
    PAS_ASSERT(context);
    if (verbose)
        pas_log("Executing constant relocations!\n");
    /* Nothing here needs to be atomic, since the constant doesn't become visible to the universe
       until the initialization context is destroyed. */
    for (index = num_relocations; index--;) {
        filc_constant_relocation* relocation;
        filc_ptr* ptr_ptr;
        relocation = relocations + index;
        PAS_ASSERT(pas_is_aligned(relocation->offset, FILC_WORD_SIZE));
        ptr_ptr = (filc_ptr*)((char*)constant + relocation->offset);
        PAS_ASSERT(filc_ptr_is_totally_null(*ptr_ptr));
        PAS_ASSERT(pas_is_aligned((uintptr_t)ptr_ptr, FILC_WORD_SIZE));
        filc_ptr_store_without_barrier(
            ptr_ptr, get_constant_value(relocation->kind, relocation->target, context));
    }
}

static bool did_run_deferred_global_ctors = false;
static void (**deferred_global_ctors)(PIZLONATED_SIGNATURE) = NULL; 
static size_t num_deferred_global_ctors = 0;
static size_t deferred_global_ctors_capacity = 0;

void filc_defer_or_run_global_ctor(void (*global_ctor)(PIZLONATED_SIGNATURE))
{
    filc_thread* my_thread = filc_get_my_thread();
    
    if (did_run_deferred_global_ctors) {
        filc_enter(my_thread);
        filc_return_buffer return_buffer;
        filc_lock_top_native_frame(my_thread);
        global_ctor(my_thread, filc_ptr_forge_null(), filc_ptr_for_int_return_buffer(&return_buffer));
        filc_unlock_top_native_frame(my_thread);
        filc_exit(my_thread);
        return;
    }

    if (num_deferred_global_ctors >= deferred_global_ctors_capacity) {
        void (**new_deferred_global_ctors)(PIZLONATED_SIGNATURE);
        size_t new_deferred_global_ctors_capacity;

        PAS_ASSERT(num_deferred_global_ctors == deferred_global_ctors_capacity);

        new_deferred_global_ctors_capacity = (deferred_global_ctors_capacity + 1) * 2;
        new_deferred_global_ctors = (void (**)(PIZLONATED_SIGNATURE))bmalloc_allocate(
            new_deferred_global_ctors_capacity * sizeof(void (*)(PIZLONATED_SIGNATURE)));

        memcpy(new_deferred_global_ctors, deferred_global_ctors,
               num_deferred_global_ctors * sizeof(void (*)(PIZLONATED_SIGNATURE)));

        bmalloc_deallocate(deferred_global_ctors);

        deferred_global_ctors = new_deferred_global_ctors;
        deferred_global_ctors_capacity = new_deferred_global_ctors_capacity;
    }

    deferred_global_ctors[num_deferred_global_ctors++] = global_ctor;
}

void filc_run_deferred_global_ctors(filc_thread* my_thread)
{
    FILC_CHECK(
        !did_run_deferred_global_ctors,
        NULL,
        "cannot run deferred global constructors twice.");
    did_run_deferred_global_ctors = true;
    /* It's important to run the destructors in exactly the order in which they were deferred, since
       this allows us to match the priority semantics despite not having the priority. */
    for (size_t index = 0; index < num_deferred_global_ctors; ++index) {
        filc_return_buffer return_buffer;
        filc_lock_top_native_frame(my_thread);
        deferred_global_ctors[index](
            my_thread, filc_ptr_forge_null(), filc_ptr_for_int_return_buffer(&return_buffer));
        filc_unlock_top_native_frame(my_thread);
    }
    bmalloc_deallocate(deferred_global_ctors);
    num_deferred_global_ctors = 0;
    deferred_global_ctors_capacity = 0;
}

void filc_run_global_dtor(void (*global_dtor)(PIZLONATED_SIGNATURE))
{
    filc_thread* my_thread = filc_get_my_thread();
    
    filc_enter(my_thread);
    filc_return_buffer return_buffer;
    filc_lock_top_native_frame(my_thread);
    global_dtor(my_thread, filc_ptr_forge_null(), filc_ptr_for_int_return_buffer(&return_buffer));
    filc_unlock_top_native_frame(my_thread);
    filc_exit(my_thread);
}

void filc_native_zrun_deferred_global_ctors(filc_thread* my_thread)
{
    filc_run_deferred_global_ctors(my_thread);
}

void filc_thread_dump_stack(filc_thread* thread, pas_stream* stream)
{
    filc_frame* frame;
    for (frame = thread->top_frame; frame; frame = frame->parent) {
        pas_stream_printf(stream, "    ");
        filc_origin_dump(frame->origin, stream);
        pas_stream_printf(stream, "\n");
    }
}

static void panic_impl(
    const filc_origin* origin, const char* prefix, const char* kind_string, const char* format,
    va_list args)
{
    filc_thread* my_thread = filc_get_my_thread();
    if (origin && my_thread->top_frame)
        my_thread->top_frame->origin = origin;
    pas_log("%s: ", prefix);
    pas_vlog(format, args);
    pas_log("\n");
    filc_thread_dump_stack(my_thread, &pas_log_stream.base);
    if (exit_on_panic) {
        pas_log("[%d] filc panic: %s\n", pas_getpid(), kind_string);
        _exit(42);
        PAS_ASSERT(!"Should not be reached");
    }
    pas_panic("%s\n", kind_string);
}

void filc_safety_panic(const filc_origin* origin, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    panic_impl(
        origin, "filc safety error", "thwarted a futile attempt to violate memory safety.", format, args);
}

void filc_internal_panic(const filc_origin* origin, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    panic_impl(origin, "filc internal error", "internal Fil-C error (it's probably a bug).", format, args);
}

void filc_user_panic(const filc_origin* origin, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    panic_impl(origin, "filc user error", "user thwarted themselves.", format, args);
}

void filc_error(const char* reason, const filc_origin* origin)
{
    filc_safety_panic(origin, "%s", reason);
}

void filc_system_mutex_lock(filc_thread* my_thread, pas_system_mutex* lock)
{
    if (pas_system_mutex_trylock(lock))
        return;

    filc_exit(my_thread);
    pas_system_mutex_lock(lock);
    filc_enter(my_thread);
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

void filc_native_zprint(filc_thread* my_thread, filc_ptr str_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    char* str = filc_check_and_get_new_str(str_ptr);
    print_str(str);
    bmalloc_deallocate(str);
}

void filc_native_zprint_long(filc_thread* my_thread, long x)
{
    PAS_UNUSED_PARAM(my_thread);
    char buf[100];
    snprintf(buf, sizeof(buf), "%ld", x);
    print_str(buf);
}

void filc_native_zprint_ptr(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    pas_allocation_config allocation_config;
    pas_string_stream stream;
    initialize_bmalloc_allocation_config(&allocation_config);
    pas_string_stream_construct(&stream, &allocation_config);
    filc_ptr_dump(ptr, &stream.base);
    print_str(pas_string_stream_get_string(&stream));
    pas_string_stream_destruct(&stream);
}

void filc_native_zerror(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    char* str = filc_check_and_get_new_str(ptr);
    filc_user_panic(NULL, "%s", str);
}

size_t filc_native_zstrlen(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    char* str = filc_check_and_get_new_str(ptr);
    size_t result = strlen(str);
    bmalloc_deallocate(str);
    return result;
}

int filc_native_zisdigit(filc_thread* my_thread, int chr)
{
    PAS_UNUSED_PARAM(my_thread);
    return isdigit(chr);
}

void filc_native_zfence(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    pas_fence();
}

void filc_native_zstore_store_fence(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    pas_store_store_fence();
}

void filc_native_zcompiler_fence(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    pas_compiler_fence(); /* lmao we don't need this */
}

bool filc_native_zunfenced_weak_cas_ptr(
    filc_thread* my_thread, filc_ptr ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_unfenced_weak_cas(my_thread, filc_ptr_ptr(ptr), expected, new_value);
}

bool filc_native_zweak_cas_ptr(filc_thread* my_thread, filc_ptr ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_weak_cas(my_thread, filc_ptr_ptr(ptr), expected, new_value);
}

filc_ptr filc_native_zunfenced_strong_cas_ptr(
    filc_thread* my_thread, filc_ptr ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_unfenced_strong_cas(my_thread, filc_ptr_ptr(ptr), expected, new_value);
}

filc_ptr filc_native_zstrong_cas_ptr(
    filc_thread* my_thread, filc_ptr ptr, filc_ptr expected, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_strong_cas(my_thread, filc_ptr_ptr(ptr), expected, new_value);
}

filc_ptr filc_native_zunfenced_xchg_ptr(filc_thread* my_thread, filc_ptr ptr, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_unfenced_xchg(my_thread, filc_ptr_ptr(ptr), new_value);
}

filc_ptr filc_native_zxchg_ptr(filc_thread* my_thread, filc_ptr ptr, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_xchg(my_thread, filc_ptr_ptr(ptr), new_value);
}

void filc_native_zatomic_store_ptr(filc_thread* my_thread, filc_ptr ptr, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    filc_ptr_store_fenced(my_thread, filc_ptr_ptr(ptr), new_value);
}

void filc_native_zunfenced_atomic_store_ptr(filc_thread* my_thread, filc_ptr ptr, filc_ptr new_value)
{
    filc_check_access_ptr(ptr, NULL);
    filc_ptr_store(my_thread, filc_ptr_ptr(ptr), new_value);
}

filc_ptr filc_native_zatomic_load_ptr(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_load_fenced_with_manual_tracking(filc_ptr_ptr(ptr));
}

filc_ptr filc_native_zunfenced_atomic_load_ptr(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    filc_check_access_ptr(ptr, NULL);
    return filc_ptr_load_with_manual_tracking(filc_ptr_ptr(ptr));
}

bool filc_native_zis_runtime_testing_enabled(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    return !!PAS_ENABLE_TESTING;
}

void filc_native_zvalidate_ptr(filc_thread* my_thread, filc_ptr ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    filc_validate_ptr(ptr, NULL);
}

void filc_native_zgc_request_and_wait(filc_thread* my_thread)
{
    static const bool verbose = false;
    if (verbose)
        pas_log("Requesting GC and waiting.\n");
    filc_exit(my_thread);
    fugc_wait(fugc_request_fresh());
    filc_enter(my_thread);
    if (verbose)
        pas_log("Done with GC.\n");
}

void filc_native_zscavenge_synchronously(filc_thread* my_thread)
{
    filc_exit(my_thread);
    pas_scavenger_run_synchronously_now();
    filc_enter(my_thread);
}

void filc_native_zscavenger_suspend(filc_thread* my_thread)
{
    filc_exit(my_thread);
    pas_scavenger_suspend();
    filc_enter(my_thread);
}

void filc_native_zscavenger_resume(filc_thread* my_thread)
{
    filc_exit(my_thread);
    pas_scavenger_resume();
    filc_enter(my_thread);
}

static void (*pizlonated_errno_handler)(PIZLONATED_SIGNATURE);

void filc_native_zregister_sys_errno_handler(filc_thread* my_thread, filc_ptr errno_handler)
{
    PAS_UNUSED_PARAM(my_thread);
    FILC_CHECK(
        !pizlonated_errno_handler,
        NULL,
        "errno handler already registered.");
    filc_check_function_call(errno_handler);
    pizlonated_errno_handler = (void(*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(errno_handler);
}

static void set_musl_errno(int errno_value)
{
    FILC_CHECK(
        pizlonated_errno_handler,
        NULL,
        "errno handler not registered when trying to set errno = %d.", errno_value);
    filc_thread* my_thread = filc_get_my_thread();
    filc_ptr args = filc_ptr_create(my_thread, filc_allocate_int(my_thread, sizeof(int)));
    *(int*)filc_ptr_ptr(args) = errno_value;
    filc_return_buffer return_buffer;
    filc_ptr rets = filc_ptr_for_int_return_buffer(&return_buffer);
    filc_lock_top_native_frame(my_thread);
    pizlonated_errno_handler(my_thread, args, rets);
    filc_unlock_top_native_frame(my_thread);
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

static void to_musl_winsize(struct winsize* winsize, struct musl_winsize* musl_winsize)
{
    musl_winsize->ws_row = winsize->ws_row;
    musl_winsize->ws_col = winsize->ws_col;
    musl_winsize->ws_xpixel = winsize->ws_xpixel;
    musl_winsize->ws_ypixel = winsize->ws_ypixel;
}

static void from_musl_winsize(struct musl_winsize* musl_winsize, struct winsize* winsize)
{
    winsize->ws_row = musl_winsize->ws_row;
    winsize->ws_col = musl_winsize->ws_col;
    winsize->ws_xpixel = musl_winsize->ws_xpixel;
    winsize->ws_ypixel = musl_winsize->ws_ypixel;
}

int filc_native_zsys_ioctl(filc_thread* my_thread, int fd, unsigned long request, filc_ptr args)
{
    static const bool verbose = false;
    
    int my_errno;
    bool in_filc = true;
    
    // NOTE: This must use musl's ioctl numbers, and must treat the passed-in struct as having the
    // pizlonated musl layout.
    switch (request) {
    case 0x5401: /* TCGETS */ {
        struct termios termios;
        struct musl_termios* musl_termios;
        filc_ptr musl_termios_ptr;
        musl_termios_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_access_int(musl_termios_ptr, sizeof(struct musl_termios), NULL);
        musl_termios = (struct musl_termios*)filc_ptr_ptr(musl_termios_ptr);
        filc_exit(my_thread);
        if (tcgetattr(fd, &termios) < 0)
            goto error;
        filc_enter(my_thread);
        to_musl_termios(&termios, musl_termios);
        return 0;
    }
    case 0x5402: /* TCGETS */
    case 0x5403: /* TCGETSW */
    case 0x5404: /* TCGETSF */ {
        struct termios termios;
        struct musl_termios* musl_termios;
        filc_ptr musl_termios_ptr;
        musl_termios_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_access_int(musl_termios_ptr, sizeof(struct musl_termios), NULL);
        musl_termios = (struct musl_termios*)filc_ptr_ptr(musl_termios_ptr);
        if (!from_musl_termios(musl_termios, &termios)) {
            errno = EINVAL;
            goto error_in_filc;
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
        filc_exit(my_thread);
        if (tcsetattr(fd, optional_actions, &termios) < 0)
            goto error;
        filc_enter(my_thread);
        return 0;
    }
    case 0x540E: /* TIOCSCTTY */ {
        filc_ptr arg_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        if (filc_ptr_ptr(arg_ptr))
            filc_check_access_int(arg_ptr, sizeof(int), NULL);
        filc_pin(filc_ptr_object(arg_ptr));
        filc_exit(my_thread);
        int result = ioctl(fd, TIOCSCTTY, filc_ptr_ptr(arg_ptr));
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        filc_unpin(filc_ptr_object(arg_ptr));
        return result;
    }
    case 0x5413: /* TIOCGWINSZ */ {
        filc_ptr musl_winsize_ptr;
        struct musl_winsize* musl_winsize;
        struct winsize winsize;
        musl_winsize_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_access_int(musl_winsize_ptr, sizeof(struct musl_winsize), NULL);
        musl_winsize = (struct musl_winsize*)filc_ptr_ptr(musl_winsize_ptr);
        filc_exit(my_thread);
        int result = ioctl(fd, TIOCGWINSZ, &winsize);
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        to_musl_winsize(&winsize, musl_winsize);
        return result;
    }
    case 0x5414: /* TIOCSWINSZ */ {
        filc_ptr musl_winsize_ptr;
        struct musl_winsize* musl_winsize;
        struct winsize winsize;
        musl_winsize_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_access_int(musl_winsize_ptr, sizeof(struct musl_winsize), NULL);
        musl_winsize = (struct musl_winsize*)filc_ptr_ptr(musl_winsize_ptr);
        from_musl_winsize(musl_winsize, &winsize);
        filc_exit(my_thread);
        int result = ioctl(fd, TIOCSWINSZ, &winsize);
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        return result;
    }
    case 0x5422: /* TIOCNOTTY */ {
        filc_exit(my_thread);
        int result = ioctl(fd, TIOCNOTTY, NULL);
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        return result;
    }
    default:
        if (verbose)
            pas_log("Invalid ioctl request = %lu.\n", request);
        set_errno(EINVAL);
        return -1;
    }

error:
    in_filc = false;
error_in_filc:
    my_errno = errno;
    if (verbose)
        pas_log("failed ioctl: %s\n", strerror(my_errno));
    if (!in_filc)
        filc_enter(my_thread);
    set_errno(my_errno);
    return -1;
}

struct musl_iovec { filc_ptr iov_base; size_t iov_len; };

static struct iovec* prepare_iovec(filc_thread* my_thread, filc_ptr musl_iov, int iovcnt)
{
    struct iovec* iov;
    size_t iov_size;
    size_t index;
    FILC_CHECK(
        iovcnt >= 0,
        NULL,
        "iovcnt cannot be negative; iovcnt = %d.\n", iovcnt);
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(struct iovec), iovcnt, &iov_size),
        NULL,
        "iovcnt too large, leading to overflow; iovcnt = %d.\n", iovcnt);
    iov = bmalloc_allocate_zeroed(iov_size);
    for (index = 0; index < (size_t)iovcnt; ++index) {
        filc_ptr musl_iov_entry;
        filc_ptr musl_iov_base;
        size_t iov_len;
        musl_iov_entry = filc_ptr_with_offset(musl_iov, sizeof(struct musl_iovec) * index);
        filc_check_access_ptr(
            filc_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_base)), NULL);
        filc_check_access_int(
            filc_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_len)),
            sizeof(size_t), NULL);
        musl_iov_base = filc_ptr_load(
            my_thread, &((struct musl_iovec*)filc_ptr_ptr(musl_iov_entry))->iov_base);
        iov_len = ((struct musl_iovec*)filc_ptr_ptr(musl_iov_entry))->iov_len;
        filc_check_access_int(musl_iov_base, iov_len, NULL);
        filc_pin(filc_ptr_object(musl_iov_base));
        iov[index].iov_base = filc_ptr_ptr(musl_iov_base);
        iov[index].iov_len = iov_len;
    }
    return iov;
}

static void unprepare_iovec(struct iovec* iov, filc_ptr musl_iov, int iovcnt)
{
    size_t index;
    for (index = 0; index < (size_t)iovcnt; ++index) {
        filc_ptr musl_iov_entry;
        filc_ptr musl_iov_base;
        musl_iov_entry = filc_ptr_with_offset(musl_iov, sizeof(struct musl_iovec) * index);
        filc_check_access_ptr(
            filc_ptr_with_offset(musl_iov_entry, PAS_OFFSETOF(struct musl_iovec, iov_base)), NULL);
        musl_iov_base = filc_ptr_load_with_manual_tracking(
            &((struct musl_iovec*)filc_ptr_ptr(musl_iov_entry))->iov_base);
        filc_unpin(filc_ptr_object(musl_iov_base));
    }
    bmalloc_deallocate(iov);
}

ssize_t filc_native_zsys_writev(filc_thread* my_thread, int fd, filc_ptr musl_iov, int iovcnt)
{
    ssize_t result;
    struct iovec* iov = prepare_iovec(my_thread, musl_iov, iovcnt);
    filc_exit(my_thread);
    result = writev(fd, iov, iovcnt);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    unprepare_iovec(iov, musl_iov, iovcnt);
    return result;
}

ssize_t filc_native_zsys_read(filc_thread* my_thread, int fd, filc_ptr buf, size_t size)
{
    filc_check_access_int(buf, size, NULL);
    filc_pin(filc_ptr_object(buf));
    filc_exit(my_thread);
    ssize_t result = read(fd, filc_ptr_ptr(buf), size);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

ssize_t filc_native_zsys_readv(filc_thread* my_thread, int fd, filc_ptr musl_iov, int iovcnt)
{
    ssize_t result;
    struct iovec* iov = prepare_iovec(my_thread, musl_iov, iovcnt);
    filc_exit(my_thread);
    result = readv(fd, iov, iovcnt);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    unprepare_iovec(iov, musl_iov, iovcnt);
    return result;
}

ssize_t filc_native_zsys_write(filc_thread* my_thread, int fd, filc_ptr buf, size_t size)
{
    filc_check_access_int(buf, size, NULL);
    filc_pin(filc_ptr_object(buf));
    filc_exit(my_thread);
    ssize_t result = write(fd, filc_ptr_ptr(buf), size);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_close(filc_thread* my_thread, int fd)
{
    filc_exit(my_thread);
    int result = close(fd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

long filc_native_zsys_lseek(filc_thread* my_thread, int fd, long offset, int whence)
{
    filc_exit(my_thread);
    long result = lseek(fd, offset, whence);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

void filc_native_zsys_exit(filc_thread* my_thread, int return_code)
{
    filc_exit(my_thread);
    _exit(return_code);
    PAS_ASSERT(!"Should not be reached");
}

unsigned filc_native_zsys_getuid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    unsigned result = getuid();
    filc_enter(my_thread);
    return result;
}

unsigned filc_native_zsys_geteuid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    unsigned result = geteuid();
    filc_enter(my_thread);
    return result;
}

unsigned filc_native_zsys_getgid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    unsigned result = getgid();
    filc_enter(my_thread);
    return result;
}

unsigned filc_native_zsys_getegid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    unsigned result = getegid();
    filc_enter(my_thread);
    return result;
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

int filc_native_zsys_open(filc_thread* my_thread, filc_ptr path_ptr, int musl_flags, filc_ptr args)
{
    int flags = from_musl_open_flags(musl_flags);
    int mode = 0;
    if (flags >= 0 && (flags & O_CREAT))
        mode = filc_ptr_get_next_int(&args);
    if (flags < 0) {
        set_errno(EINVAL);
        return -1;
    }
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = open(path, flags, mode);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    bmalloc_deallocate(path);
    return result;
}

int filc_native_zsys_getpid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    int result = getpid();
    filc_enter(my_thread);
    return result;
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

int filc_native_zsys_clock_gettime(filc_thread* my_thread, int musl_clock_id, filc_ptr timespec_ptr)
{
    filc_check_access_int(timespec_ptr, sizeof(struct musl_timespec), NULL);
    clockid_t clock_id;
    if (!from_musl_clock_id(musl_clock_id, &clock_id)) {
        set_errno(EINVAL);
        return -1;
    }
    struct timespec ts;
    filc_exit(my_thread);
    int result = clock_gettime(clock_id, &ts);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        return -1;
    }
    struct musl_timespec* musl_timespec = (struct musl_timespec*)filc_ptr_ptr(timespec_ptr);
    musl_timespec->tv_sec = ts.tv_sec;
    musl_timespec->tv_nsec = ts.tv_nsec;
    return 0;
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

static int handle_fstat_result(filc_ptr musl_stat_ptr, struct stat *st,
                               int result, int my_errno)
{
    filc_check_access_int(musl_stat_ptr, sizeof(struct musl_stat), NULL);
    if (result < 0) {
        set_errno(my_errno);
        return -1;
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
    return 0;
}

int filc_native_zsys_fstatat(
    filc_thread* my_thread, int fd, filc_ptr path_ptr, filc_ptr musl_stat_ptr, int musl_flag)
{
    int flag;
    if (!from_musl_fstatat_flag(musl_flag, &flag)) {
        set_errno(EINVAL);
        return -1;
    }
    if (fd == -100)
        fd = AT_FDCWD;
    struct stat st;
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = fstatat(fd, path, &st, flag);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(path);
    return handle_fstat_result(musl_stat_ptr, &st, result, my_errno);
}

int filc_native_zsys_fstat(filc_thread* my_thread, int fd, filc_ptr musl_stat_ptr)
{
    struct stat st;
    filc_exit(my_thread);
    int result = fstat(fd, &st);
    int my_errno = errno;
    filc_enter(my_thread);
    return handle_fstat_result(musl_stat_ptr, &st, result, my_errno);
}

struct musl_flock {
    short l_type;
    short l_whence;
    int64_t l_start;
    int64_t l_len;
    int l_pid;
};

int filc_native_zsys_fcntl(filc_thread* my_thread, int fd, int musl_cmd, filc_ptr args)
{
    static const bool verbose = false;
    
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
        arg_int = filc_ptr_get_next_int(&args);
    if (have_arg_flock) {
        musl_flock_ptr = filc_ptr_get_next_ptr(my_thread, &args);
        filc_check_access_int(musl_flock_ptr, sizeof(struct musl_flock), NULL);
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
            set_errno(EINVAL);
            return -1;
        }
        arg_flock.l_whence = musl_flock->l_whence;
        arg_flock.l_start = musl_flock->l_start;
        arg_flock.l_len = musl_flock->l_len;
        arg_flock.l_pid = musl_flock->l_pid;
    }
    if (!have_cmd) {
        if (verbose)
            pas_log("no cmd!\n");
        set_errno(EINVAL);
        return -1;
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
            return -1;
        }
        break;
    case F_SETFL:
        arg_int = from_musl_open_flags(arg_int);
        break;
    default:
        break;
    }
    int result;
    filc_exit(my_thread);
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
    filc_enter(my_thread);
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
    return result;
}

static struct filc_ptr to_musl_passwd(filc_thread* my_thread, struct passwd* passwd)
{
    filc_ptr result_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(struct musl_passwd)));
    check_musl_passwd(result_ptr);

    struct musl_passwd* result = (struct musl_passwd*)filc_ptr_ptr(result_ptr);

    filc_ptr_store(my_thread, &result->pw_name, filc_strdup(my_thread, passwd->pw_name));
    filc_ptr_store(my_thread, &result->pw_passwd, filc_strdup(my_thread, passwd->pw_passwd));
    result->pw_uid = passwd->pw_uid;
    result->pw_gid = passwd->pw_gid;
    filc_ptr_store(my_thread, &result->pw_gecos, filc_strdup(my_thread, passwd->pw_gecos));
    filc_ptr_store(my_thread, &result->pw_dir, filc_strdup(my_thread, passwd->pw_dir));
    filc_ptr_store(my_thread, &result->pw_shell, filc_strdup(my_thread, passwd->pw_shell));
    
    return result_ptr;
}

filc_ptr filc_native_zsys_getpwuid(filc_thread* my_thread, unsigned uid)
{
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct passwd* passwd = getpwuid(uid);
    if (!passwd) {
        set_errno(errno);
        return filc_ptr_forge_null();
    }
    return to_musl_passwd(my_thread, passwd);
}

static void from_musl_sigset(struct musl_sigset* musl_sigset,
                             sigset_t* sigset)
{
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    pas_reasonably_fill_sigset(sigset);
    
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

static void check_signal(int signum)
{
    FILC_CHECK(signum != SIGILL, NULL, "cannot override SIGILL.");
    FILC_CHECK(signum != SIGTRAP, NULL, "cannot override SIGTRAP.");
    FILC_CHECK(signum != SIGBUS, NULL, "cannot override SIGBUS.");
    FILC_CHECK(signum != SIGSEGV, NULL, "cannot override SIGSEGV.");
    FILC_CHECK(signum != SIGFPE, NULL, "cannot override SIGFPE.");
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

int filc_native_zsys_sigaction(filc_thread* my_thread, int musl_signum, filc_ptr act_ptr, filc_ptr oact_ptr)
{
    static const bool verbose = false;
    
    int signum = from_musl_signum(musl_signum);
    if (signum < 0) {
        if (verbose)
            pas_log("bad signum\n");
        set_errno(EINVAL);
        return -1;
    }
    check_signal(signum);
    if (filc_ptr_ptr(act_ptr))
        check_musl_sigaction(act_ptr);
    if (filc_ptr_ptr(oact_ptr))
        check_musl_sigaction(oact_ptr);
    struct musl_sigaction* musl_act = (struct musl_sigaction*)filc_ptr_ptr(act_ptr);
    struct musl_sigaction* musl_oact = (struct musl_sigaction*)filc_ptr_ptr(oact_ptr);
    struct sigaction act;
    struct sigaction oact;
    if (musl_act) {
        from_musl_sigset(&musl_act->sa_mask, &act.sa_mask);
        filc_ptr musl_handler = filc_ptr_load(my_thread, &musl_act->sa_handler_ish);
        if (is_musl_special_signal_handler(filc_ptr_ptr(musl_handler)))
            act.sa_handler = from_musl_special_signal_handler(filc_ptr_ptr(musl_handler));
        else {
            filc_check_function_call(musl_handler);
            filc_object* handler_object = filc_allocate_special(
                my_thread, sizeof(filc_signal_handler), FILC_WORD_TYPE_SIGNAL_HANDLER);
            filc_thread_track_object(my_thread, handler_object);
            filc_signal_handler* handler = (filc_signal_handler*)handler_object->lower;
            handler->function_ptr = musl_handler;
            handler->mask = act.sa_mask;
            handler->musl_signum = musl_signum;
            pas_store_store_fence();
            PAS_ASSERT((unsigned)musl_signum <= FILC_MAX_MUSL_SIGNUM);
            filc_store_barrier(my_thread, filc_object_for_special_payload(handler));
            signal_table[musl_signum] = handler;
            act.sa_handler = signal_pizlonator;
        }
        if (!from_musl_sa_flags(musl_act->sa_flags, &act.sa_flags)) {
            set_errno(EINVAL);
            return -1;
        }
    }
    if (musl_oact)
        pas_zero_memory(&oact, sizeof(struct sigaction));
    filc_exit(my_thread);
    int result = sigaction(signum, musl_act ? &act : NULL, musl_oact ? &oact : NULL);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        return -1;
    }
    if (musl_oact) {
        if (is_special_signal_handler(oact.sa_handler))
            filc_ptr_store(my_thread, &musl_oact->sa_handler_ish,
                           to_musl_special_signal_handler(oact.sa_handler));
        else {
            PAS_ASSERT(oact.sa_handler == signal_pizlonator);
            PAS_ASSERT((unsigned)musl_signum <= FILC_MAX_MUSL_SIGNUM);
            /* FIXME: The signal_table entry should really be a filc_ptr so we can return it here. */
            filc_ptr_store(my_thread, &musl_oact->sa_handler_ish,
                           filc_ptr_load_with_manual_tracking(&signal_table[musl_signum]->function_ptr));
        }
        to_musl_sigset(&oact.sa_mask, &musl_oact->sa_mask);
        musl_oact->sa_flags = to_musl_sa_flags(oact.sa_flags);
    }
    return 0;
}

int filc_native_zsys_isatty(filc_thread* my_thread, int fd)
{
    filc_exit(my_thread);
    errno = 0;
    int result = isatty(fd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (!result && my_errno)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_pipe(filc_thread* my_thread, filc_ptr fds_ptr)
{
    filc_check_access_int(fds_ptr, sizeof(int) * 2, NULL);
    int fds[2];
    filc_exit(my_thread);
    int result = pipe(fds);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        /* Make sure not to modify what fds_ptr points to on error, even if the system modified
           our fds, since that would be nonconforming behavior. Probably doesn't matter since of
           course we would never run on a nonconforming system. */
        set_errno(my_errno);
        return -1;
    }
    ((int*)filc_ptr_ptr(fds_ptr))[0] = fds[0];
    ((int*)filc_ptr_ptr(fds_ptr))[1] = fds[1];
    return 0;
}

struct musl_timeval {
    uint64_t tv_sec;
    uint64_t tv_usec;
};

int filc_native_zsys_select(filc_thread* my_thread, int nfds,
                            filc_ptr readfds_ptr, filc_ptr writefds_ptr, filc_ptr exceptfds_ptr,
                            filc_ptr timeout_ptr)
{
    PAS_ASSERT(FD_SETSIZE == 1024);
    FILC_CHECK(
        nfds <= 1024,
        NULL,
        "attempt to select with nfds = %d (should be 1024 or less).",
        nfds);
    if (filc_ptr_ptr(readfds_ptr))
        filc_check_access_int(readfds_ptr, sizeof(fd_set), NULL);
    if (filc_ptr_ptr(writefds_ptr))
        filc_check_access_int(writefds_ptr, sizeof(fd_set), NULL);
    if (filc_ptr_ptr(exceptfds_ptr))
        filc_check_access_int(exceptfds_ptr, sizeof(fd_set), NULL);
    if (filc_ptr_ptr(timeout_ptr))
        filc_check_access_int(timeout_ptr, sizeof(struct musl_timeval), NULL);
    fd_set* readfds = (fd_set*)filc_ptr_ptr(readfds_ptr);
    fd_set* writefds = (fd_set*)filc_ptr_ptr(writefds_ptr);
    fd_set* exceptfds = (fd_set*)filc_ptr_ptr(exceptfds_ptr);
    struct musl_timeval* musl_timeout = (struct musl_timeval*)filc_ptr_ptr(timeout_ptr);
    struct timeval timeout;
    if (musl_timeout) {
        timeout.tv_sec = musl_timeout->tv_sec;
        timeout.tv_usec = musl_timeout->tv_usec;
    }
    filc_pin(filc_ptr_object(readfds_ptr));
    filc_pin(filc_ptr_object(writefds_ptr));
    filc_pin(filc_ptr_object(exceptfds_ptr));
    filc_exit(my_thread);
    int result = select(nfds, readfds, writefds, exceptfds, musl_timeout ? &timeout : NULL);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(readfds_ptr));
    filc_unpin(filc_ptr_object(writefds_ptr));
    filc_unpin(filc_ptr_object(exceptfds_ptr));
    if (result < 0)
        set_errno(my_errno);
    if (musl_timeout) {
        musl_timeout->tv_sec = timeout.tv_sec;
        musl_timeout->tv_usec = timeout.tv_usec;
    }
    return result;
}

void filc_native_zsys_sched_yield(filc_thread* my_thread)
{
    filc_exit(my_thread);
    sched_yield();
    filc_enter(my_thread);
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

int filc_native_zsys_socket(filc_thread* my_thread, int musl_domain, int musl_type, int protocol)
{
    /* the protocol constants seem to align between Darwin and musl. */
    int domain;
    if (!from_musl_domain(musl_domain, &domain)) {
        set_errno(EINVAL);
        return -1;
    }
    int type;
    if (!from_musl_socket_type(musl_type, &type)) {
        set_errno(EINVAL);
        return -1;
    }
    filc_exit(my_thread);
    int result = socket(domain, type, protocol);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
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

static bool to_musl_socket_level(int level, int* result)
{
    if (level == SOL_SOCKET)
        *result = 1;
    else
        *result = level;
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

int filc_native_zsys_setsockopt(filc_thread* my_thread, int sockfd, int musl_level, int musl_optname,
                                filc_ptr optval_ptr, unsigned optlen)
{
    int level;
    if (!from_musl_socket_level(musl_level, &level))
        goto einval;
    int optname;
    /* for now, all of the possible arguments are primitives. */
    filc_check_access_int(optval_ptr, optlen, NULL);
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
    filc_pin(filc_ptr_object(optval_ptr));
    filc_exit(my_thread);
    int result = setsockopt(sockfd, level, optname, optval, optlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(optval_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
    
enoprotoopt:
    set_errno(ENOPROTOOPT);
    return -1;
    
einval:
    set_errno(EINVAL);
    return -1;
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
                               struct sockaddr** addr, unsigned* addrlen)
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
            bmalloc_allocate_zeroed(sizeof(struct sockaddr_in));
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
            bmalloc_allocate_zeroed(sizeof(struct sockaddr_un));
        pas_zero_memory(addr_un, sizeof(struct sockaddr_un));
        addr_un->sun_family = PF_LOCAL;
        char* sun_path = filc_check_and_get_new_str_for_int_memory(
            musl_addr_un->sun_path, sizeof(musl_addr_un->sun_path));
        snprintf(addr_un->sun_path, sizeof(addr_un->sun_path), "%s", sun_path);
        bmalloc_deallocate(sun_path);
        *addr = (struct sockaddr*)addr_un;
        *addrlen = sizeof(struct sockaddr_un);
        return true;
    }
    case PF_INET6: {
        if (musl_addrlen < sizeof(struct musl_sockaddr_in6))
            return false;
        struct musl_sockaddr_in6* musl_addr_in6 = (struct musl_sockaddr_in6*)musl_addr;
        struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)
            bmalloc_allocate_zeroed(sizeof(struct sockaddr_in6));
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

static void ensure_musl_sockaddr(filc_thread* my_thread,
                                 filc_ptr* musl_addr_ptr, unsigned* musl_addrlen,
                                 unsigned desired_musl_addrlen)
{
    PAS_ASSERT(musl_addr_ptr);
    PAS_ASSERT(musl_addrlen);
    PAS_ASSERT(filc_ptr_is_totally_null(*musl_addr_ptr));
    PAS_ASSERT(!*musl_addrlen);
    
    *musl_addr_ptr = filc_ptr_create(my_thread, filc_allocate_int(my_thread, desired_musl_addrlen));
    *musl_addrlen = desired_musl_addrlen;
}

static bool to_new_musl_sockaddr(filc_thread* my_thread,
                                 struct sockaddr* addr, unsigned addrlen,
                                 filc_ptr* musl_addr_ptr, unsigned* musl_addrlen)
{
    int musl_family;
    if (!to_musl_domain(addr->sa_family, &musl_family))
        return false;
    switch (addr->sa_family) {
    case PF_INET: {
        PAS_ASSERT(addrlen >= sizeof(struct sockaddr_in));
        ensure_musl_sockaddr(my_thread, musl_addr_ptr, musl_addrlen, sizeof(struct musl_sockaddr_in));
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(*musl_addr_ptr);
        pas_zero_memory(musl_addr, sizeof(struct musl_sockaddr_in));
        struct musl_sockaddr_in* musl_addr_in = (struct musl_sockaddr_in*)musl_addr;
        struct sockaddr_in* addr_in = (struct sockaddr_in*)addr;
        musl_addr_in->sin_family = musl_family;
        musl_addr_in->sin_port = addr_in->sin_port;
        musl_addr_in->sin_addr = addr_in->sin_addr.s_addr;
        return true;
    }
    case PF_LOCAL: {
        PAS_ASSERT(addrlen >= sizeof(struct sockaddr_un));
        ensure_musl_sockaddr(my_thread, musl_addr_ptr, musl_addrlen, sizeof(struct musl_sockaddr_un));
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(*musl_addr_ptr);
        pas_zero_memory(musl_addr, sizeof(struct musl_sockaddr_un));
        struct musl_sockaddr_un* musl_addr_un = (struct musl_sockaddr_un*)musl_addr;
        struct sockaddr_un* addr_un = (struct sockaddr_un*)addr;
        musl_addr_un->sun_family = musl_family;
        PAS_ASSERT(sizeof(addr_un->sun_path) == sizeof(musl_addr_un->sun_path));
        memcpy(musl_addr_un->sun_path, addr_un->sun_path, sizeof(musl_addr_un->sun_path));
        return true;
    }
    case PF_INET6: {
        PAS_ASSERT(addrlen >= sizeof(struct sockaddr_in6));
        ensure_musl_sockaddr(my_thread, musl_addr_ptr, musl_addrlen, sizeof(struct musl_sockaddr_in6));
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(*musl_addr_ptr);
        pas_zero_memory(musl_addr, sizeof(struct musl_sockaddr_in6));
        struct musl_sockaddr_in6* musl_addr_in6 = (struct musl_sockaddr_in6*)musl_addr;
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

static bool to_musl_sockaddr(filc_thread* my_thread,
                             struct sockaddr* addr, unsigned addrlen,
                             struct musl_sockaddr* musl_addr, unsigned* musl_addrlen)
{
    PAS_ASSERT(musl_addrlen);
    
    filc_ptr temp_musl_addr_ptr = filc_ptr_forge_null();
    unsigned temp_musl_addrlen = 0;

    if (!to_new_musl_sockaddr(my_thread, addr, addrlen, &temp_musl_addr_ptr, &temp_musl_addrlen)) {
        PAS_ASSERT(filc_ptr_is_totally_null(temp_musl_addr_ptr));
        PAS_ASSERT(!temp_musl_addrlen);
        return false;
    }

    if (!musl_addr)
        PAS_ASSERT(!*musl_addrlen);

    memcpy(musl_addr, filc_ptr_ptr(temp_musl_addr_ptr), pas_min_uintptr(*musl_addrlen, temp_musl_addrlen));
    *musl_addrlen = temp_musl_addrlen;
    return true;
}

int filc_native_zsys_bind(filc_thread* my_thread, int sockfd, filc_ptr musl_addr_ptr, unsigned musl_addrlen)
{
    static const bool verbose = false;
    
    filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);
    if (musl_addrlen < sizeof(struct musl_sockaddr))
        goto einval;
    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    struct sockaddr* addr;
    unsigned addrlen;
    if (!from_musl_sockaddr(musl_addr, musl_addrlen, &addr, &addrlen))
        goto einval;

    if (verbose)
        pas_log("calling bind!\n");
    filc_exit(my_thread);
    int result = bind(sockfd, addr, addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);

    bmalloc_deallocate(addr);
    return result;
    
einval:
    set_errno(EINVAL);
    return -1;
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

int filc_native_zsys_getaddrinfo(filc_thread* my_thread, filc_ptr node_ptr, filc_ptr service_ptr,
                                 filc_ptr hints_ptr, filc_ptr res_ptr)
{
    struct musl_addrinfo* musl_hints = filc_ptr_ptr(hints_ptr);
    if (musl_hints)
        check_musl_addrinfo(hints_ptr);
    filc_check_access_ptr(res_ptr, NULL);
    struct addrinfo hints;
    pas_zero_memory(&hints, sizeof(hints));
    if (musl_hints) {
        if (!from_musl_ai_flags(musl_hints->ai_flags, &hints.ai_flags))
            return to_musl_eai(EAI_BADFLAGS);
        if (!from_musl_domain(musl_hints->ai_family, &hints.ai_family))
            return to_musl_eai(EAI_FAMILY);
        if (!from_musl_socket_type(musl_hints->ai_socktype, &hints.ai_socktype))
            return to_musl_eai(EAI_SOCKTYPE);
        hints.ai_protocol = musl_hints->ai_protocol;
        if (musl_hints->ai_addrlen
            || filc_ptr_ptr(filc_ptr_load_with_manual_tracking(&musl_hints->ai_addr))
            || filc_ptr_ptr(filc_ptr_load_with_manual_tracking(&musl_hints->ai_canonname))
            || filc_ptr_ptr(filc_ptr_load_with_manual_tracking(&musl_hints->ai_next))) {
            errno = EINVAL;
            return to_musl_eai(EAI_SYSTEM);
        }
    }
    char* node = filc_check_and_get_new_str_or_null(node_ptr);
    char* service = filc_check_and_get_new_str_or_null(service_ptr);
    struct addrinfo* res = NULL;
    int result;
    int musl_result = 0;
    filc_exit(my_thread);
    result = getaddrinfo(node, service, &hints, &res);
    filc_enter(my_thread);
    if (result) {
        musl_result = to_musl_eai(result);
        goto done;
    }
    filc_ptr* addrinfo_ptr = (filc_ptr*)filc_ptr_ptr(res_ptr);
    struct addrinfo* current;
    for (current = res; current; current = current->ai_next) {
        filc_ptr musl_current_ptr = filc_ptr_create(
            my_thread, filc_allocate(my_thread, sizeof(struct musl_addrinfo)));
        check_musl_addrinfo(musl_current_ptr);
        struct musl_addrinfo* musl_current = (struct musl_addrinfo*)filc_ptr_ptr(musl_current_ptr);
        filc_ptr_store(my_thread, addrinfo_ptr, musl_current_ptr);
        PAS_ASSERT(to_musl_ai_flags(current->ai_flags, &musl_current->ai_flags));
        PAS_ASSERT(to_musl_domain(current->ai_family, &musl_current->ai_family));
        PAS_ASSERT(to_musl_socket_type(current->ai_socktype, &musl_current->ai_socktype));
        musl_current->ai_protocol = current->ai_protocol;
        filc_ptr musl_addr_ptr = filc_ptr_forge_null();
        unsigned musl_addrlen = 0;
        PAS_ASSERT(to_new_musl_sockaddr(
                       my_thread, current->ai_addr, current->ai_addrlen, &musl_addr_ptr, &musl_addrlen));
        musl_current->ai_addrlen = musl_addrlen;
        filc_ptr_store(my_thread, &musl_current->ai_addr, musl_addr_ptr);
        filc_ptr_store(my_thread, &musl_current->ai_canonname,
                       filc_strdup(my_thread, current->ai_canonname));
        addrinfo_ptr = &musl_current->ai_next;
    }
    filc_ptr_store(my_thread, addrinfo_ptr, filc_ptr_forge_invalid(NULL));
    freeaddrinfo(res);

done:
    bmalloc_deallocate(node);
    bmalloc_deallocate(service);
    return musl_result;
}

int filc_native_zsys_connect(filc_thread* my_thread,
                             int sockfd, filc_ptr musl_addr_ptr, unsigned musl_addrlen)
{
    static const bool verbose = false;
    
    filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);
    if (musl_addrlen < sizeof(struct musl_sockaddr))
        goto einval;
    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    struct sockaddr* addr;
    unsigned addrlen;
    if (!from_musl_sockaddr(musl_addr, musl_addrlen, &addr, &addrlen))
        goto einval;

    filc_exit(my_thread);
    int result = connect(sockfd, addr, addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);

    bmalloc_deallocate(addr);
    if (verbose)
        pas_log("connect succeeded!\n");
    return result;
    
einval:
    set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_getsockname(filc_thread* my_thread,
                                 int sockfd, filc_ptr musl_addr_ptr, filc_ptr musl_addrlen_ptr)
{
    static const bool verbose = false;
    
    filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
    unsigned musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);

    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = (struct sockaddr*)alloca(addrlen);
    filc_exit(my_thread);
    int result = getsockname(sockfd, addr, &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        return result;
    }
    
    PAS_ASSERT(addrlen <= MAX_SOCKADDRLEN);

    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    /* pass our own copy of musl_addrlen to avoid TOCTOU. */
    PAS_ASSERT(to_musl_sockaddr(my_thread, addr, addrlen, musl_addr, &musl_addrlen));
    *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    if (verbose)
        pas_log("getsockname succeeded!\n");
    return 0;
}

int filc_native_zsys_getsockopt(filc_thread* my_thread, int sockfd, int musl_level, int musl_optname,
                                filc_ptr optval_ptr, filc_ptr optlen_ptr)
{
    static const bool verbose = false;
    
    filc_check_access_int(optlen_ptr, sizeof(unsigned), NULL);
    unsigned musl_optlen = *(unsigned*)filc_ptr_ptr(optlen_ptr);
    int level;
    if (verbose)
        pas_log("here!\n");
    if (!from_musl_socket_level(musl_level, &level))
        goto einval;
    int optname;
    /* everything is primitive, for now */
    filc_check_access_int(optval_ptr, musl_optlen, NULL);
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
    filc_pin(filc_ptr_object(optval_ptr));
    filc_exit(my_thread);
    int result = getsockopt(sockfd, level, optname, optval, &optlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(optval_ptr));
    if (result < 0) {
        set_errno(my_errno);
        return result;
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
    return 0;

enoprotoopt:
    if (verbose)
        pas_log("bad proto\n");
    set_errno(ENOPROTOOPT);
    return -1;
    
einval:
    if (verbose)
        pas_log("einval\n");
    set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_getpeername(filc_thread* my_thread, int sockfd, filc_ptr musl_addr_ptr,
                                 filc_ptr musl_addrlen_ptr)
{
    static const bool verbose = false;
    
    filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
    unsigned musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);

    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = (struct sockaddr*)alloca(addrlen);
    filc_exit(my_thread);
    int result = getpeername(sockfd, addr, &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        return result;
    }
    
    PAS_ASSERT(addrlen <= MAX_SOCKADDRLEN);

    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    /* pass our own copy of musl_addrlen to avoid TOCTOU. */
    PAS_ASSERT(to_musl_sockaddr(my_thread, addr, addrlen, musl_addr, &musl_addrlen));
    *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    if (verbose)
        pas_log("getpeername succeeded!\n");
    return 0;
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

static bool to_musl_msg_flags(int flags, int* result)
{
    *result = 0;
    if (check_and_clear(&flags, MSG_OOB))
        *result |= 0x0001;
    if (check_and_clear(&flags, MSG_PEEK))
        *result |= 0x0002;
    if (check_and_clear(&flags, MSG_DONTROUTE))
        *result |= 0x0004;
    if (check_and_clear(&flags, MSG_CTRUNC))
        *result |= 0x0008;
    if (check_and_clear(&flags, MSG_TRUNC))
        *result |= 0x0020;
    if (check_and_clear(&flags, MSG_DONTWAIT))
        *result |= 0x0040;
    if (check_and_clear(&flags, MSG_EOR))
        *result |= 0x0080;
    if (check_and_clear(&flags, MSG_WAITALL))
        *result |= 0x0100;
    if (check_and_clear(&flags, MSG_NOSIGNAL))
        *result |= 0x4000;
    return !flags;
}

ssize_t filc_native_zsys_sendto(filc_thread* my_thread, int sockfd, filc_ptr buf_ptr, size_t len,
                                int musl_flags, filc_ptr musl_addr_ptr, unsigned musl_addrlen)
{
    filc_check_access_int(buf_ptr, len, NULL);
    filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);
    int flags;
    if (!from_musl_msg_flags(musl_flags, &flags))
        goto einval;
    struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
    struct sockaddr* addr;
    unsigned addrlen;
    if (!from_musl_sockaddr(musl_addr, musl_addrlen, &addr, &addrlen))
        goto einval;
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    ssize_t result = sendto(sockfd, filc_ptr_ptr(buf_ptr), len, flags, addr, addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    if (result < 0)
        set_errno(my_errno);

    bmalloc_deallocate(addr);
    return result;

einval:
    set_errno(EINVAL);
    return -1;
}

ssize_t filc_native_zsys_recvfrom(filc_thread* my_thread, int sockfd, filc_ptr buf_ptr, size_t len,
                                  int musl_flags, filc_ptr musl_addr_ptr, filc_ptr musl_addrlen_ptr)
{
    filc_check_access_int(buf_ptr, len, NULL);
    unsigned musl_addrlen;
    if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
        musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
        filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);
    } else
        musl_addrlen = 0;
    int flags;
    if (!from_musl_msg_flags(musl_flags, &flags)) {
        set_errno(EINVAL);
        return -1;
    }
    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = NULL;
    if (filc_ptr_ptr(musl_addr_ptr)) {
        addr = (struct sockaddr*)alloca(addrlen);
        pas_zero_memory(addr, addrlen);
    }
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    ssize_t result = recvfrom(
        sockfd, filc_ptr_ptr(buf_ptr), len, flags,
        addr, filc_ptr_ptr(musl_addrlen_ptr) ? &addrlen : NULL);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    if (result < 0)
        set_errno(my_errno);
    else if (filc_ptr_ptr(musl_addrlen_ptr)) {
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
        /* pass our own copy of musl_addrlen to avoid TOCTOU. */
        PAS_ASSERT(to_musl_sockaddr(my_thread, addr, addrlen, musl_addr, &musl_addrlen));
        *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    }

    return result;
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

int filc_native_zsys_getrlimit(filc_thread* my_thread, int musl_resource, filc_ptr rlim_ptr)
{
    filc_check_access_int(rlim_ptr, sizeof(struct musl_rlimit), NULL);
    int resource;
    if (!from_musl_resource(musl_resource, &resource))
        goto einval;
    struct rlimit rlim;
    filc_exit(my_thread);
    int result = getrlimit(resource, &rlim);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    else {
        PAS_ASSERT(!result);
        struct musl_rlimit* musl_rlim = (struct musl_rlimit*)filc_ptr_ptr(rlim_ptr);
        musl_rlim->rlim_cur = to_musl_rlimit_value(rlim.rlim_cur);
        musl_rlim->rlim_max = to_musl_rlimit_value(rlim.rlim_max);
    }
    return result;

einval:
    set_errno(EINVAL);
    return -1;
}

unsigned filc_native_zsys_umask(filc_thread* my_thread, unsigned mask)
{
    filc_exit(my_thread);
    unsigned result = umask(mask);
    filc_enter(my_thread);
    return result;
}

struct musl_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

int filc_native_zsys_uname(filc_thread* my_thread, filc_ptr buf_ptr)
{
    filc_check_access_int(buf_ptr, sizeof(struct musl_utsname), NULL);
    struct musl_utsname* musl_buf = (struct musl_utsname*)filc_ptr_ptr(buf_ptr);
    struct utsname buf;
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    PAS_ASSERT(!uname(&buf));
    snprintf(musl_buf->sysname, sizeof(musl_buf->sysname), "%s", buf.sysname);
    snprintf(musl_buf->nodename, sizeof(musl_buf->nodename), "%s", buf.nodename);
    snprintf(musl_buf->release, sizeof(musl_buf->release), "%s", buf.release);
    snprintf(musl_buf->version, sizeof(musl_buf->version), "%s", buf.version);
    snprintf(musl_buf->machine, sizeof(musl_buf->machine), "%s", buf.machine);
    PAS_ASSERT(!getdomainname(musl_buf->domainname, sizeof(musl_buf->domainname) - 1));
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    musl_buf->domainname[sizeof(musl_buf->domainname) - 1] = 0;
    return 0;
}

struct musl_itimerval {
    struct musl_timeval it_interval;
    struct musl_timeval it_value;
};

int filc_native_zsys_getitimer(filc_thread* my_thread, int which, filc_ptr musl_value_ptr)
{
    filc_check_access_int(musl_value_ptr, sizeof(struct musl_itimerval), NULL);
    filc_exit(my_thread);
    struct itimerval value;
    int result = getitimer(which, &value);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        return -1;
    }
    struct musl_itimerval* musl_value = (struct musl_itimerval*)filc_ptr_ptr(musl_value_ptr);
    musl_value->it_interval.tv_sec = value.it_interval.tv_sec;
    musl_value->it_interval.tv_usec = value.it_interval.tv_usec;
    musl_value->it_value.tv_sec = value.it_value.tv_sec;
    musl_value->it_value.tv_usec = value.it_value.tv_usec;
    return 0;
}

int filc_native_zsys_setitimer(filc_thread* my_thread, int which, filc_ptr musl_new_value_ptr,
                               filc_ptr musl_old_value_ptr)
{
    filc_check_access_int(musl_new_value_ptr, sizeof(struct musl_itimerval), NULL);
    if (filc_ptr_ptr(musl_old_value_ptr))
        filc_check_access_int(musl_old_value_ptr, sizeof(struct musl_itimerval), NULL);
    struct itimerval new_value;
    struct musl_itimerval* musl_new_value = (struct musl_itimerval*)filc_ptr_ptr(musl_new_value_ptr);
    new_value.it_interval.tv_sec = musl_new_value->it_interval.tv_sec;
    new_value.it_interval.tv_usec = musl_new_value->it_interval.tv_usec;
    new_value.it_value.tv_sec = musl_new_value->it_value.tv_sec;
    new_value.it_value.tv_usec = musl_new_value->it_value.tv_usec;
    filc_exit(my_thread);
    struct itimerval old_value;
    int result = setitimer(which, &new_value, &old_value);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        return -1;
    }
    struct musl_itimerval* musl_old_value = (struct musl_itimerval*)filc_ptr_ptr(musl_old_value_ptr);
    if (musl_old_value) {
        musl_old_value->it_interval.tv_sec = old_value.it_interval.tv_sec;
        musl_old_value->it_interval.tv_usec = old_value.it_interval.tv_usec;
        musl_old_value->it_value.tv_sec = old_value.it_value.tv_sec;
        musl_old_value->it_value.tv_usec = old_value.it_value.tv_usec;
    }
    return 0;
}

int filc_native_zsys_pause(filc_thread* my_thread)
{
    filc_exit(my_thread);
    int result = pause();
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(result == -1);
    set_errno(my_errno);
    return -1;
}

int filc_native_zsys_pselect(filc_thread* my_thread, int nfds,
                             filc_ptr readfds_ptr, filc_ptr writefds_ptr, filc_ptr exceptfds_ptr,
                             filc_ptr timeout_ptr, filc_ptr sigmask_ptr)
{
    PAS_ASSERT(FD_SETSIZE == 1024);
    FILC_CHECK(
        nfds <= 1024,
        NULL,
        "attempt to select with nfds = %d (should be 1024 or less).",
        nfds);
    if (filc_ptr_ptr(readfds_ptr))
        filc_check_access_int(readfds_ptr, sizeof(fd_set), NULL);
    if (filc_ptr_ptr(writefds_ptr))
        filc_check_access_int(writefds_ptr, sizeof(fd_set), NULL);
    if (filc_ptr_ptr(exceptfds_ptr))
        filc_check_access_int(exceptfds_ptr, sizeof(fd_set), NULL);
    if (filc_ptr_ptr(timeout_ptr))
        filc_check_access_int(timeout_ptr, sizeof(struct musl_timespec), NULL);
    if (filc_ptr_ptr(sigmask_ptr))
        filc_check_access_int(sigmask_ptr, sizeof(struct musl_sigset), NULL);
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
    filc_pin(filc_ptr_object(readfds_ptr));
    filc_pin(filc_ptr_object(writefds_ptr));
    filc_pin(filc_ptr_object(exceptfds_ptr));
    filc_exit(my_thread);
    int result = pselect(nfds, readfds, writefds, exceptfds,
                         musl_timeout ? &timeout : NULL,
                         musl_sigmask ? &sigmask : NULL);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(readfds_ptr));
    filc_unpin(filc_ptr_object(writefds_ptr));
    filc_unpin(filc_ptr_object(exceptfds_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_getpeereid(filc_thread* my_thread, int fd, filc_ptr uid_ptr, filc_ptr gid_ptr)
{
    filc_check_access_int(uid_ptr, sizeof(unsigned), NULL);
    filc_check_access_int(gid_ptr, sizeof(unsigned), NULL);
    filc_exit(my_thread);
    uid_t uid;
    gid_t gid;
    int result = getpeereid(fd, &uid, &gid);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(result == -1 || !result);
    if (!result) {
        *(unsigned*)filc_ptr_ptr(uid_ptr) = uid;
        *(unsigned*)filc_ptr_ptr(gid_ptr) = gid;
        return 0;
    }
    set_errno(my_errno);
    return -1;
}

int filc_native_zsys_kill(filc_thread* my_thread, int pid, int musl_sig)
{
    int sig = from_musl_signum(musl_sig);
    if (sig < 0) {
        set_errno(EINVAL);
        return -1;
    }
    filc_exit(my_thread);
    int result = kill(pid, sig);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_raise(filc_thread* my_thread, int musl_sig)
{
    int sig = from_musl_signum(musl_sig);
    if (sig < 0) {
        set_errno(EINVAL);
        return -1;
    }
    filc_exit(my_thread);
    int result = raise(sig);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_dup(filc_thread* my_thread, int fd)
{
    filc_exit(my_thread);
    int result = dup(fd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_dup2(filc_thread* my_thread, int oldfd, int newfd)
{
    filc_exit(my_thread);
    int result = dup2(oldfd, newfd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_sigprocmask(filc_thread* my_thread, int musl_how, filc_ptr musl_set_ptr,
                                 filc_ptr musl_oldset_ptr)
{
    static const bool verbose = false;
    
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
        return -1;
    }
    sigset_t* set;
    sigset_t* oldset;
    if (filc_ptr_ptr(musl_set_ptr)) {
        filc_check_access_int(musl_set_ptr, sizeof(struct musl_sigset), NULL);
        set = alloca(sizeof(sigset_t));
        from_musl_sigset((struct musl_sigset*)filc_ptr_ptr(musl_set_ptr), set);
    } else
        set = NULL;
    if (filc_ptr_ptr(musl_oldset_ptr)) {
        filc_check_access_int(musl_oldset_ptr, sizeof(struct musl_sigset), NULL);
        oldset = alloca(sizeof(sigset_t));
        pas_zero_memory(oldset, sizeof(sigset_t));
    } else
        oldset = NULL;
    filc_exit(my_thread);
    if (verbose)
        pas_log("%s: setting sigmask\n", __PRETTY_FUNCTION__);
    int result = pthread_sigmask(how, set, oldset);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(result == -1 || !result);
    if (result < 0) {
        set_errno(my_errno);
        return -1;
    }
    if (filc_ptr_ptr(musl_oldset_ptr)) {
        PAS_ASSERT(oldset);
        to_musl_sigset(oldset, (struct musl_sigset*)filc_ptr_ptr(musl_oldset_ptr));
    }
    return 0;
}

filc_ptr filc_native_zsys_getpwnam(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_new_str(name_ptr);
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct passwd* passwd = getpwnam(name);
    int my_errno = errno;
    bmalloc_deallocate(name);
    if (!passwd) {
        set_errno(my_errno);
        return filc_ptr_forge_null();
    }
    return to_musl_passwd(my_thread, passwd);
}

int filc_native_zsys_setgroups(filc_thread* my_thread, size_t size, filc_ptr list_ptr)
{
    static const bool verbose = false;
    
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(unsigned), size, &total_size),
        NULL,
        "size argument too big, causes overflow; size = %zu.",
        size);
    filc_check_access_int(list_ptr, total_size, NULL);
    filc_pin(filc_ptr_object(list_ptr));
    filc_exit(my_thread);
    PAS_ASSERT(sizeof(gid_t) == sizeof(unsigned));
    if (verbose) {
        pas_log("size = %zu\n", size);
        pas_log("NGROUPS_MAX = %zu\n", (size_t)NGROUPS_MAX);
    }
    int result = setgroups(size, (gid_t*)filc_ptr_ptr(list_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(list_ptr));
    if (verbose)
        pas_log("result = %d, error = %s\n", result, strerror(my_errno));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

filc_ptr filc_native_zsys_opendir(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_new_str(name_ptr);
    filc_exit(my_thread);
    DIR* dir = opendir(name);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(name);
    if (!dir) {
        set_errno(my_errno);
        return filc_ptr_forge_null();
    }
    return filc_ptr_for_special_payload(my_thread, zdirstream_create(my_thread, dir));
}

filc_ptr filc_native_zsys_fdopendir(filc_thread* my_thread, int fd)
{
    filc_exit(my_thread);
    DIR* dir = fdopendir(fd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (!dir) {
        set_errno(my_errno);
        return filc_ptr_forge_null();
    }
    return filc_ptr_for_special_payload(my_thread, zdirstream_create(my_thread, dir));
}

int filc_native_zsys_closedir(filc_thread* my_thread, filc_ptr dirstream_ptr)
{
    zdirstream_check(dirstream_ptr);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);

    pas_lock_lock(&dirstream->lock);
    DIR* dir = dirstream->dir;
    FILC_ASSERT(dir, NULL);
    dirstream->dir = NULL;
    pas_allocation_config allocation_config;
    initialize_bmalloc_allocation_config(&allocation_config);
    bmalloc_deallocate(dirstream->musl_to_loc);
    pas_uint64_hash_map_destruct(&dirstream->loc_to_musl, &allocation_config);
    pas_lock_unlock(&dirstream->lock);
    filc_free_yolo(my_thread, filc_object_for_special_payload(dirstream));

    filc_exit(my_thread);
    int result = closedir(dir);
    int my_errno = errno;
    filc_enter(my_thread);

    if (result < 0)
        set_errno(my_errno);
    return result;
}

static uint64_t to_musl_dirstream_loc(zdirstream* stream, uint64_t loc)
{
    pas_uint64_hash_map_add_result add_result;
    pas_allocation_config allocation_config;

    PAS_ASSERT(stream->loc_to_musl.table_size == stream->musl_to_loc_size);
        
    initialize_bmalloc_allocation_config(&allocation_config);

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

            new_musl_to_loc = bmalloc_allocate_zeroed(total_size);
            memcpy(new_musl_to_loc, stream->musl_to_loc, sizeof(uint64_t) * stream->musl_to_loc_size);
            bmalloc_deallocate(stream->musl_to_loc);
            stream->musl_to_loc = new_musl_to_loc;
            stream->musl_to_loc_capacity = new_capacity;
        }
        PAS_ASSERT(stream->musl_to_loc_size < stream->musl_to_loc_capacity);

        stream->musl_to_loc[stream->musl_to_loc_size++] = loc;
    }

    return add_result.entry->value;
}

static DIR* begin_using_dirstream_holding_lock(zdirstream* dirstream)
{
    pas_lock_assert_held(&dirstream->lock);
    /* If this fails, it's because of a race to call readdir, which is not supported. */
    FILC_ASSERT(!dirstream->in_use, NULL);
    DIR* dir = dirstream->dir;
    FILC_ASSERT(dir, NULL);
    dirstream->in_use = true;
    return dir;
}

static DIR* begin_using_dirstream(zdirstream* dirstream)
{
    pas_lock_lock(&dirstream->lock);
    DIR* dir = begin_using_dirstream_holding_lock(dirstream);
    pas_lock_unlock(&dirstream->lock);
    return dir;
}

static void end_using_dirstream_holding_lock(zdirstream* dirstream, DIR* dir)
{
    pas_lock_assert_held(&dirstream->lock);
    PAS_ASSERT(dirstream->in_use);
    PAS_ASSERT(dirstream->dir == dir);
    dirstream->in_use = false;
}

static void end_using_dirstream(zdirstream* dirstream, DIR* dir)
{
    pas_lock_lock(&dirstream->lock);
    end_using_dirstream_holding_lock(dirstream, dir);
    pas_lock_unlock(&dirstream->lock);
}

filc_ptr filc_native_zsys_readdir(filc_thread* my_thread, filc_ptr dirstream_ptr)
{
    zdirstream_check(dirstream_ptr);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    DIR* dir = begin_using_dirstream(dirstream);

    filc_exit(my_thread);
    /* FIXME: Maybe we can break readdir in some disastrous way if we reenter via a signal and call it
       again?
    
       But if we did, Fil-C would hit us with the in_use check. */
    struct dirent* result = readdir(dir);
    int my_errno = errno;
    filc_enter(my_thread);

    filc_ptr dirent_ptr = filc_ptr_forge_null();
    if (result) {
        dirent_ptr = filc_ptr_create(my_thread, filc_allocate_int(my_thread, sizeof(struct musl_dirent)));
        struct musl_dirent* dirent = (struct musl_dirent*)filc_ptr_ptr(dirent_ptr);
        dirent->d_ino = result->d_ino;
        dirent->d_reclen = result->d_reclen;
        dirent->d_type = result->d_type; /* Amazingly, these constants line up. */
        snprintf(dirent->d_name, sizeof(dirent->d_name), "%s", result->d_name);
    }

    end_using_dirstream(dirstream, dir);
    return dirent_ptr;
}

void filc_native_zsys_rewinddir(filc_thread* my_thread, filc_ptr dirstream_ptr)
{
    zdirstream_check(dirstream_ptr);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    DIR* dir = begin_using_dirstream(dirstream);
    
    filc_exit(my_thread);
    rewinddir(dir);
    filc_enter(my_thread);

    end_using_dirstream(dirstream, dir);
}

void filc_native_zsys_seekdir(filc_thread* my_thread, filc_ptr dirstream_ptr, long musl_loc)
{
    zdirstream_check(dirstream_ptr);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    pas_lock_lock(&dirstream->lock);
    DIR* dir = begin_using_dirstream_holding_lock(dirstream);
    FILC_ASSERT((uint64_t)musl_loc < (uint64_t)dirstream->musl_to_loc_size, NULL);
    long loc = dirstream->musl_to_loc[musl_loc];
    pas_lock_unlock(&dirstream->lock);
    
    filc_exit(my_thread);
    seekdir(dir, loc);
    filc_enter(my_thread);

    end_using_dirstream(dirstream, dir);
}

long filc_native_zsys_telldir(filc_thread* my_thread, filc_ptr dirstream_ptr)
{
    zdirstream_check(dirstream_ptr);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    DIR* dir = begin_using_dirstream(dirstream);
    filc_exit(my_thread);
    long loc = telldir(dir);
    int my_errno = errno;
    filc_enter(my_thread);
    pas_lock_lock(&dirstream->lock);
    long musl_loc = -1;
    if (loc >= 0)
        musl_loc = to_musl_dirstream_loc(dirstream, loc);
    end_using_dirstream_holding_lock(dirstream, dir);
    pas_lock_unlock(&dirstream->lock);
    if (loc < 0)
        set_errno(my_errno);
    return musl_loc;
}

int filc_native_zsys_dirfd(filc_thread* my_thread, filc_ptr dirstream_ptr)
{
    zdirstream_check(dirstream_ptr);
    zdirstream* dirstream = (zdirstream*)filc_ptr_ptr(dirstream_ptr);
    DIR* dir = begin_using_dirstream(dirstream);
    
    filc_exit(my_thread);
    int result = dirfd(dir);
    int my_errno = errno;
    filc_enter(my_thread);

    end_using_dirstream(dirstream, dir);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

void filc_native_zsys_closelog(filc_thread* my_thread)
{
    filc_exit(my_thread);
    closelog();
    filc_enter(my_thread);
}

void filc_native_zsys_openlog(filc_thread* my_thread, filc_ptr ident_ptr, int option, int facility)
{
    char* ident = filc_check_and_get_new_str(ident_ptr);
    filc_exit(my_thread);
    /* Amazingly, the option/facility constants all match up! */
    openlog(ident, option, facility);
    filc_enter(my_thread);
    bmalloc_deallocate(ident);
}

int filc_native_zsys_setlogmask(filc_thread* my_thread, int mask)
{
    filc_exit(my_thread);
    int result = setlogmask(mask);
    filc_enter(my_thread);
    return result;
}

void filc_native_zsys_syslog(filc_thread* my_thread, int priority, filc_ptr msg_ptr)
{
    char* msg = filc_check_and_get_new_str(msg_ptr);
    filc_exit(my_thread);
    syslog(priority, "%s", msg);
    filc_enter(my_thread);
    bmalloc_deallocate(msg);
}

int filc_native_zsys_chdir(filc_thread* my_thread, filc_ptr path_ptr)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = chdir(path);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(path);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_fork(filc_thread* my_thread)
{
    static const bool verbose = false;
    filc_exit(my_thread);
    if (verbose)
        pas_log("suspending scavenger\n");
    pas_scavenger_suspend();
    if (verbose)
        pas_log("suspending GC\n");
    fugc_suspend();
    if (!fugc_world_is_stopped) {
        if (verbose)
            pas_log("stopping world\n");
        filc_stop_the_world();
    }
    /* NOTE: We don't have to lock the soft handshake lock, since now that the world is stopped and the
       FUGC is suspended, nobody could be using it. */
    if (verbose)
        pas_log("locking the parking lot\n");
    void* parking_lot_cookie = filc_parking_lot_lock();
    if (verbose)
        pas_log("locking thread list\n");
    filc_thread_list_lock_lock();
    pas_lock_disallowed = true;
    filc_thread* thread;
    for (thread = filc_first_thread; thread; thread = thread->next_thread)
        pas_system_mutex_lock(&thread->lock);
    int result = fork();
    int my_errno = errno;
    pas_lock_disallowed = false;
    if (verbose)
        pas_log("fork result = %d\n", result);
    if (!result) {
        /* We're in the child. Make sure that the thread list only contains the current thread and that
           the other threads know that they are dead due to fork. */
        for (thread = filc_first_thread; thread;) {
            filc_thread* next_thread = thread->next_thread;
            thread->prev_thread = NULL;
            thread->next_thread = NULL;
            if (thread != my_thread)
                thread->forked = true;
            pas_system_mutex_unlock(&thread->lock);
            thread = next_thread;
        }
        filc_first_thread = my_thread;
        PAS_ASSERT(filc_first_thread == my_thread);
        PAS_ASSERT(!filc_first_thread->next_thread);
        PAS_ASSERT(!filc_first_thread->prev_thread);

        /* FIXME: Maybe reuse tids??? */
    } else {
        for (thread = filc_first_thread; thread; thread = thread->next_thread)
            pas_system_mutex_unlock(&thread->lock);
    }
    filc_thread_list_lock_unlock();
    filc_parking_lot_unlock(parking_lot_cookie);
    if (!fugc_world_is_stopped)
        filc_resume_the_world();
    fugc_resume();
    pas_scavenger_resume();
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
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

int filc_native_zsys_waitpid(filc_thread* my_thread, int pid, filc_ptr status_ptr, int options)
{
    filc_exit(my_thread);
    int status;
    int result = waitpid(pid, &status, options);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        PAS_ASSERT(result == -1);
        set_errno(my_errno);
        return -1;
    }
    if (filc_ptr_ptr(status_ptr)) {
        filc_check_access_int(status_ptr, sizeof(int), NULL);
        *(int*)filc_ptr_ptr(status_ptr) = to_musl_wait_status(status);
    }
    return result;
}

int filc_native_zsys_listen(filc_thread* my_thread, int sockfd, int backlog)
{
    filc_exit(my_thread);
    int result = listen(sockfd, backlog);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_accept(filc_thread* my_thread, int sockfd, filc_ptr musl_addr_ptr,
                            filc_ptr musl_addrlen_ptr)
{
    unsigned musl_addrlen;
    if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_access_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
        musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
        filc_check_access_int(musl_addr_ptr, musl_addrlen, NULL);
    } else
        musl_addrlen = 0;
    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = NULL;
    if (filc_ptr_ptr(musl_addr_ptr)) {
        addr = (struct sockaddr*)alloca(addrlen);
        pas_zero_memory(addr, addrlen);
    }
    filc_exit(my_thread);
    int result = accept(sockfd, addr, filc_ptr_ptr(musl_addrlen_ptr) ? &addrlen : NULL);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    else {
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
        /* pass our own copy of musl_addrlen to avoid TOCTOU. */
        PAS_ASSERT(to_musl_sockaddr(my_thread, addr, addrlen, musl_addr, &musl_addrlen));
        *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    }
    return result;
}

int filc_native_zsys_socketpair(filc_thread* my_thread, int musl_domain, int musl_type, int protocol,
                                filc_ptr sv_ptr)
{
    filc_check_access_int(sv_ptr, sizeof(int) * 2, NULL);
    int domain;
    if (!from_musl_domain(musl_domain, &domain)) {
        set_errno(EINVAL);
        return -1;
    }
    int type;
    if (!from_musl_socket_type(musl_type, &type)) {
        set_errno(EINVAL);
        return -1;
    }
    filc_pin(filc_ptr_object(sv_ptr));
    filc_exit(my_thread);
    int result = socketpair(domain, type, protocol, (int*)filc_ptr_ptr(sv_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(sv_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_setsid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    int result = setsid();
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

static size_t length_of_null_terminated_ptr_array(filc_ptr array_ptr)
{
    size_t result = 0;
    for (;; result++) {
        filc_check_access_ptr(array_ptr, NULL);
        if (!filc_ptr_ptr(filc_ptr_load_with_manual_tracking((filc_ptr*)filc_ptr_ptr(array_ptr))))
            return result;
        array_ptr = filc_ptr_with_offset(array_ptr, sizeof(filc_ptr));
    }
}

int filc_native_zsys_execve(filc_thread* my_thread, filc_ptr pathname_ptr, filc_ptr argv_ptr,
                            filc_ptr envp_ptr)
{
    char* pathname = filc_check_and_get_new_str(pathname_ptr);
    size_t argv_len = length_of_null_terminated_ptr_array(argv_ptr);
    size_t envp_len = length_of_null_terminated_ptr_array(envp_ptr);
    char** argv = (char**)bmalloc_allocate_zeroed((argv_len + 1) * sizeof(char*));
    char** envp = (char**)bmalloc_allocate_zeroed((envp_len + 1) * sizeof(char*));
    argv[argv_len] = NULL;
    envp[envp_len] = NULL;
    size_t index;
    for (index = argv_len; index--;) {
        argv[index] = filc_check_and_get_new_str(
            filc_ptr_load(my_thread, (filc_ptr*)filc_ptr_ptr(argv_ptr) + index));
    }
    for (index = envp_len; index--;) {
        envp[index] = filc_check_and_get_new_str(
            filc_ptr_load(my_thread, (filc_ptr*)filc_ptr_ptr(envp_ptr) + index));
    }
    filc_exit(my_thread);
    int result = execve(pathname, argv, envp);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(result == -1);
    set_errno(my_errno);
    for (index = argv_len; index--;)
        bmalloc_deallocate(argv[index]);
    for (index = envp_len; index--;)
        bmalloc_deallocate(envp[index]);
    bmalloc_deallocate(argv);
    bmalloc_deallocate(envp);
    bmalloc_deallocate(pathname);
    return -1;
}

int filc_native_zsys_getppid(filc_thread* my_thread)
{
    filc_exit(my_thread);
    int result = getppid();
    filc_enter(my_thread);
    return result;
}

int filc_native_zsys_chroot(filc_thread* my_thread, filc_ptr path_ptr)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = chroot(path);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(path);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_setuid(filc_thread* my_thread, unsigned uid)
{
    filc_exit(my_thread);
    int result = setuid(uid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_seteuid(filc_thread* my_thread, unsigned uid)
{
    filc_exit(my_thread);
    int result = seteuid(uid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_setreuid(filc_thread* my_thread, unsigned ruid, unsigned euid)
{
    filc_exit(my_thread);
    int result = setreuid(ruid, euid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_setgid(filc_thread* my_thread, unsigned gid)
{
    filc_exit(my_thread);
    int result = setgid(gid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_setegid(filc_thread* my_thread, unsigned gid)
{
    filc_exit(my_thread);
    int result = setegid(gid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_setregid(filc_thread* my_thread, unsigned rgid, unsigned egid)
{
    filc_exit(my_thread);
    int result = setregid(rgid, egid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_nanosleep(filc_thread* my_thread, filc_ptr musl_req_ptr, filc_ptr musl_rem_ptr)
{
    filc_check_access_int(musl_req_ptr, sizeof(struct musl_timespec), NULL);
    if (filc_ptr_ptr(musl_rem_ptr))
        filc_check_access_int(musl_rem_ptr, sizeof(struct musl_timespec), NULL);
    struct timespec req;
    struct timespec rem;
    req.tv_sec = ((struct musl_timespec*)filc_ptr_ptr(musl_req_ptr))->tv_sec;
    req.tv_nsec = ((struct musl_timespec*)filc_ptr_ptr(musl_req_ptr))->tv_nsec;
    filc_exit(my_thread);
    int result = nanosleep(&req, &rem);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        set_errno(my_errno);
        if (my_errno == EINTR && filc_ptr_ptr(musl_rem_ptr)) {
            ((struct musl_timespec*)filc_ptr_ptr(musl_rem_ptr))->tv_sec = rem.tv_sec;
            ((struct musl_timespec*)filc_ptr_ptr(musl_rem_ptr))->tv_nsec = rem.tv_nsec;
        }
    }
    return result;
}

int filc_native_zsys_getgroups(filc_thread* my_thread, int size, filc_ptr list_ptr)
{
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(unsigned), size, &total_size),
        NULL,
        "size argument too big, causes overflow; size = %d.",
        size);
    filc_check_access_int(list_ptr, total_size, NULL);
    filc_pin(filc_ptr_object(list_ptr));
    filc_exit(my_thread);
    PAS_ASSERT(sizeof(gid_t) == sizeof(unsigned));
    int result = getgroups(size, (gid_t*)filc_ptr_ptr(list_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(list_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_getgrouplist(filc_thread* my_thread, filc_ptr user_ptr, unsigned group,
                                  filc_ptr groups_ptr, filc_ptr ngroups_ptr)
{
    static const bool verbose = false;
    
    char* user = filc_check_and_get_new_str(user_ptr);
    filc_check_access_int(ngroups_ptr, sizeof(int), NULL);
    int ngroups = *(int*)filc_ptr_ptr(ngroups_ptr);
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(unsigned), ngroups, &total_size),
        NULL,
        "ngroups argument too big, causes overflow; ngroups = %d.",
        ngroups);
    filc_check_access_int(groups_ptr, total_size, NULL);
    filc_pin(filc_ptr_object(groups_ptr));
    filc_exit(my_thread);
    if (verbose)
        pas_log("ngroups = %d\n", ngroups);
    int result = getgrouplist(user, group, (int*)filc_ptr_ptr(groups_ptr), &ngroups);
    if (verbose)
        pas_log("ngroups after = %d\n", ngroups);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(groups_ptr));
    bmalloc_deallocate(user);
    if (verbose)
        pas_log("result = %d, error = %s\n", result, strerror(my_errno));
    if (result < 0)
        set_errno(my_errno);
    *(int*)filc_ptr_ptr(ngroups_ptr) = ngroups;
    return result;
}

int filc_native_zsys_initgroups(filc_thread* my_thread, filc_ptr user_ptr, unsigned gid)
{
    static const bool verbose = false;
    
    char* user = filc_check_and_get_new_str(user_ptr);
    filc_exit(my_thread);
    int result = initgroups(user, gid);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(user);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

long filc_native_zsys_readlink(filc_thread* my_thread, filc_ptr path_ptr, filc_ptr buf_ptr, size_t bufsize)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_check_access_int(buf_ptr, bufsize, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    long result = readlink(path, (char*)filc_ptr_ptr(buf_ptr), bufsize);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    bmalloc_deallocate(path);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_openpty(filc_thread* my_thread, filc_ptr masterfd_ptr, filc_ptr slavefd_ptr,
                             filc_ptr name_ptr, filc_ptr musl_term_ptr, filc_ptr musl_win_ptr)
{
    filc_check_access_int(masterfd_ptr, sizeof(int), NULL);
    filc_check_access_int(slavefd_ptr, sizeof(int), NULL);
    FILC_ASSERT(!filc_ptr_ptr(name_ptr), NULL);
    struct termios* term = NULL;
    if (filc_ptr_ptr(musl_term_ptr)) {
        filc_check_access_int(musl_term_ptr, sizeof(struct musl_termios), NULL);
        term = alloca(sizeof(struct termios));
        from_musl_termios((struct musl_termios*)filc_ptr_ptr(musl_term_ptr), term);
    }
    struct winsize* win = NULL;
    if (filc_ptr_ptr(musl_win_ptr)) {
        filc_check_access_int(musl_win_ptr, sizeof(struct musl_winsize), NULL);
        win = alloca(sizeof(struct winsize));
        from_musl_winsize((struct musl_winsize*)filc_ptr_ptr(musl_win_ptr), win);
    }
    filc_pin(filc_ptr_object(masterfd_ptr));
    filc_pin(filc_ptr_object(slavefd_ptr));
    filc_exit(my_thread);
    int result = openpty((int*)filc_ptr_ptr(masterfd_ptr), (int*)filc_ptr_ptr(slavefd_ptr),
                         NULL, term, win);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(masterfd_ptr));
    filc_unpin(filc_ptr_object(slavefd_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_ttyname_r(filc_thread* my_thread, int fd, filc_ptr buf_ptr, size_t buflen)
{
    filc_check_access_int(buf_ptr, buflen, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    int result = ttyname_r(fd, (char*)filc_ptr_ptr(buf_ptr), buflen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

static struct filc_ptr to_musl_group(filc_thread* my_thread, struct group* group)
{
    filc_ptr result_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(struct musl_group)));
    check_musl_group(result_ptr);
    struct musl_group* result = (struct musl_group*)filc_ptr_ptr(result_ptr);

    filc_ptr_store(my_thread, &result->gr_name, filc_strdup(my_thread, group->gr_name));
    filc_ptr_store(my_thread, &result->gr_passwd, filc_strdup(my_thread, group->gr_passwd));
    result->gr_gid = group->gr_gid;
    
    if (group->gr_mem) {
        size_t size = 0;
        while (group->gr_mem[size++]);

        size_t total_size;
        FILC_ASSERT(!pas_mul_uintptr_overflow(sizeof(filc_ptr), size, &total_size), NULL);
        filc_ptr musl_mem_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, total_size));
        filc_ptr* musl_mem = (filc_ptr*)filc_ptr_ptr(musl_mem_ptr);
        size_t index;
        for (index = 0; index < size; ++index) {
            filc_check_access_ptr(filc_ptr_with_ptr(musl_mem_ptr, musl_mem + index), NULL);
            filc_ptr_store(my_thread, musl_mem + index, filc_strdup(my_thread, group->gr_mem[index]));
        }
        filc_ptr_store(my_thread, &result->gr_mem, musl_mem_ptr);
    }
    
    return result_ptr;
}

filc_ptr filc_native_zsys_getgrnam(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_new_str(name_ptr);
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct group* group = getgrnam(name);
    int my_errno = errno;
    bmalloc_deallocate(name);
    if (!group) {
        set_errno(my_errno);
        return filc_ptr_forge_null();
    }
    /* FIXME: a signal that calls getgrnam here could cause a reentrancy issue, maybe.
     
       We could address that with a reentrancy "lock" per thread that is just a bit saying, "we're doing
       stuff to grnam on this thread so if you try to do it then get fucked". */
    return to_musl_group(my_thread, group);
}

int filc_native_zsys_chown(filc_thread* my_thread, filc_ptr pathname_ptr, unsigned owner, unsigned group)
{
    char* pathname = filc_check_and_get_new_str(pathname_ptr);
    filc_exit(my_thread);
    int result = chown(pathname, owner, group);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(pathname);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_chmod(filc_thread* my_thread, filc_ptr pathname_ptr, unsigned mode)
{
    char* pathname = filc_check_and_get_new_str(pathname_ptr);
    filc_exit(my_thread);
    int result = chmod(pathname, mode);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(pathname);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

/* If me or someone else decides to give a shit then maybe like this whole thing, and other things like it
   in this file, would die in a fire.
   
   - We should probably just go all-in on Fil-C being a Linux thing.
   
   - In that case, it would be straightforward to sync up the structs between pizlonated code and legacy
     code. Only structs that have pointers in them would require translation!
   
   - And code like login() would just be fully pizlonated because libutil would be fully pizlonated.
   
   - So much code in this file would die if we did that!
   
   Why am I not doing that? Because I'm on a roll getting shit to work and it's fun, so who cares. */
struct musl_utmpx {
    short ut_type;
    short __ut_pad1;
    int ut_pid;
    char ut_line[32];
    char ut_id[4];
    char ut_user[32];
    char ut_host[256];
    struct {
        short __e_termination;
        short __e_exit;
    } ut_exit;
    int ut_session, __ut_pad2;
    struct musl_timeval ut_tv;
    unsigned ut_addr_v6[4];
};

static void from_musl_utmpx(struct musl_utmpx* musl_utmpx, struct utmpx* utmpx)
{
    /* Just a reminder about why this is how it must be:
       
       Fil-C is multithreaded and we do not have ownership concepts of any kind.
       
       Therefore, if we have a pointer to a mutable data structure, then for all we know, so does some
       other thread. And for all we know, it's writing to the data structure right now.
       
       So: if pizlonated code gives us a string, or a struct that has a string, then:
       
       - We must be paranoid about what it looks like right now. For all we know, there's no null
         terminator.
       
       - We must be paranoid about what it will look like at every instant in time between now and
         whenever we're done with it, and whenever any syscalls or system functions we pass that string
         to are done with it.
       
       To make this super simple, we always clone out the string using filc_check_and_get_new_str. That
       function is race-safe. It'll either return a new string that is null-terminated without having to
       overrun the pointer's capability, or it will kill the shit out of the process.
       
       If you still don't get it, here's a race that we totally guard against:
       
       1. Two threads have access to the passed-in utmpx pointer.
       
       2. One thread calls login.
       
       3. Then we check that the ut_line string is null-terminated within the bounds (32 bytes, currently).
       
       4. Then we start to copy the ut_line string into our utmp.
       
       5. Then the other thread wakes up and overwrites the utmpx with a bunch of nonzeroes.
       
       6. Now our copy loop will overrun the end of the passed-in utmpx! Memory safety violation not
          thwarted, if that happened!
       
       But it won't happen, because filc_check_and_get_new_str() is a thing and we remembered to call
       it. That function will trap if it loses that race. If it wins the race, we get a null-terminated
       string of the right length. */
    char* line = filc_check_and_get_new_str_for_int_memory(musl_utmpx->ut_line,
                                                           sizeof(musl_utmpx->ut_line));
    char* user = filc_check_and_get_new_str_for_int_memory(musl_utmpx->ut_user,
                                                           sizeof(musl_utmpx->ut_user));
    char* host = filc_check_and_get_new_str_for_int_memory(musl_utmpx->ut_host,
                                                           sizeof(musl_utmpx->ut_host));
    pas_zero_memory(utmpx, sizeof(struct utmpx));
    snprintf(utmpx->ut_line, sizeof(utmpx->ut_line), "%s", line);
    snprintf(utmpx->ut_user, sizeof(utmpx->ut_user), "%s", user);
    snprintf(utmpx->ut_host, sizeof(utmpx->ut_host), "%s", host);
    PAS_ASSERT(sizeof(utmpx->ut_id) == sizeof(musl_utmpx->ut_id));
    memcpy(utmpx->ut_id, musl_utmpx->ut_id, sizeof(musl_utmpx->ut_id));
    utmpx->ut_tv.tv_sec = musl_utmpx->ut_tv.tv_sec;
    utmpx->ut_tv.tv_usec = musl_utmpx->ut_tv.tv_usec;
    utmpx->ut_pid = musl_utmpx->ut_pid;
    switch (musl_utmpx->ut_type) {
    case 0:
        utmpx->ut_type = EMPTY;
        break;
    case 1:
        utmpx->ut_type = RUN_LVL;
        break;
    case 2:
        utmpx->ut_type = BOOT_TIME;
        break;
    case 3:
        utmpx->ut_type = NEW_TIME;
        break;
    case 4:
        utmpx->ut_type = OLD_TIME;
        break;
    case 5:
        utmpx->ut_type = INIT_PROCESS;
        break;
    case 6:
        utmpx->ut_type = LOGIN_PROCESS;
        break;
    case 7:
        utmpx->ut_type = USER_PROCESS;
        break;
    case 8:
        utmpx->ut_type = DEAD_PROCESS;
        break;
    default:
        PAS_ASSERT(!"Unrecognized utmpx->ut_type");
        break;
    }
    bmalloc_deallocate(line);
    bmalloc_deallocate(user);
    bmalloc_deallocate(host);
}

static void to_musl_utmpx(struct utmpx* utmpx, struct musl_utmpx* musl_utmpx)
{
    pas_zero_memory(musl_utmpx, sizeof(struct musl_utmpx));
    
    /* I trust nothing and nobody!!! */
    utmpx->ut_user[sizeof(utmpx->ut_user) - 1] = 0;
    utmpx->ut_id[sizeof(utmpx->ut_id) - 1] = 0;
    utmpx->ut_line[sizeof(utmpx->ut_line) - 1] = 0;
    utmpx->ut_host[sizeof(utmpx->ut_host) - 1] = 0;

    snprintf(musl_utmpx->ut_line, sizeof(musl_utmpx->ut_line), "%s", utmpx->ut_line);
    snprintf(musl_utmpx->ut_user, sizeof(musl_utmpx->ut_user), "%s", utmpx->ut_user);
    snprintf(musl_utmpx->ut_host, sizeof(musl_utmpx->ut_host), "%s", utmpx->ut_host);
    snprintf(musl_utmpx->ut_id, sizeof(musl_utmpx->ut_id), "%s", utmpx->ut_id);
    musl_utmpx->ut_tv.tv_sec = utmpx->ut_tv.tv_sec;
    musl_utmpx->ut_tv.tv_usec = utmpx->ut_tv.tv_usec;
    musl_utmpx->ut_pid = utmpx->ut_pid;
    switch (utmpx->ut_type) {
    case EMPTY:
        musl_utmpx->ut_type = 0;
        break;
    case RUN_LVL:
        musl_utmpx->ut_type = 1;
        break;
    case BOOT_TIME:
        musl_utmpx->ut_type = 2;
        break;
    case NEW_TIME:
        musl_utmpx->ut_type = 3;
        break;
    case OLD_TIME:
        musl_utmpx->ut_type = 4;
        break;
    case INIT_PROCESS:
        musl_utmpx->ut_type = 5;
        break;
    case LOGIN_PROCESS:
        musl_utmpx->ut_type = 6;
        break;
    case USER_PROCESS:
        musl_utmpx->ut_type = 7;
        break;
    case DEAD_PROCESS:
        musl_utmpx->ut_type = 8;
        break;
    default:
        PAS_ASSERT(!"Unrecognized utmpx->ut_type");
        break;
    }
}

/* We currently do utmpx logic under global lock and without exiting. This is to protect against memory
   safety issues that would arise if utmpx API was called in a racy or reentrant way. The issues would
   happen inside our calls to the underlying system functions.

   In the future, we probably want to do exactly this, but with exiting (and that means acquiring the
   lock in an exit-friendly way).

   One implication of the current approach is that if multiple threads use utmpx API then we totally
   might livelock in soft handshake:

   - Thread #1 grabs this lock and exits.
   - Soft handshake is requested.
   - Thread #2 tries to grab this lock while entered.

   I think that's fine, since that's a memory-safe outcome, and anyway you shouldn't be using this API
   from multiple threads. Ergo, broken programs might get fucked in a memory-safe way. */
static pas_lock utmpx_lock = PAS_LOCK_INITIALIZER;

static filc_ptr handle_utmpx_result(filc_thread* my_thread, struct utmpx* utmpx)
{
    if (!utmpx) {
        set_errno(errno);
        return filc_ptr_forge_null();
    }
    filc_ptr result = filc_ptr_create(my_thread, filc_allocate_int(my_thread, sizeof(struct musl_utmpx)));
    to_musl_utmpx(utmpx, (struct musl_utmpx*)filc_ptr_ptr(result));
    return result;
}

void filc_native_zsys_endutxent(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    pas_lock_lock(&utmpx_lock);
    endutxent();
    pas_lock_unlock(&utmpx_lock);
}

filc_ptr filc_native_zsys_getutxent(filc_thread* my_thread)
{
    pas_lock_lock(&utmpx_lock);
    filc_ptr result = handle_utmpx_result(my_thread, getutxent());
    pas_lock_unlock(&utmpx_lock);
    return result;
}

filc_ptr filc_native_zsys_getutxid(filc_thread* my_thread, filc_ptr musl_utmpx_ptr)
{
    filc_check_access_int(musl_utmpx_ptr, sizeof(struct musl_utmpx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct utmpx utmpx_in;
    from_musl_utmpx((struct musl_utmpx*)filc_ptr_ptr(musl_utmpx_ptr), &utmpx_in);
    filc_ptr result = handle_utmpx_result(my_thread, getutxid(&utmpx_in));
    pas_lock_unlock(&utmpx_lock);
    return result;
}

filc_ptr filc_native_zsys_getutxline(filc_thread* my_thread, filc_ptr musl_utmpx_ptr)
{
    filc_check_access_int(musl_utmpx_ptr, sizeof(struct musl_utmpx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct utmpx utmpx_in;
    from_musl_utmpx((struct musl_utmpx*)filc_ptr_ptr(musl_utmpx_ptr), &utmpx_in);
    filc_ptr result = handle_utmpx_result(my_thread, getutxline(&utmpx_in));
    pas_lock_unlock(&utmpx_lock);
    return result;
}

filc_ptr filc_native_zsys_pututxline(filc_thread* my_thread, filc_ptr musl_utmpx_ptr)
{
    filc_check_access_int(musl_utmpx_ptr, sizeof(struct musl_utmpx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct utmpx utmpx_in;
    from_musl_utmpx((struct musl_utmpx*)filc_ptr_ptr(musl_utmpx_ptr), &utmpx_in);
    struct utmpx* utmpx_out = pututxline(&utmpx_in);
    filc_ptr result;
    if (utmpx_out == &utmpx_in)
        result = musl_utmpx_ptr;
    else
        result = handle_utmpx_result(my_thread, utmpx_out);
    pas_lock_unlock(&utmpx_lock);
    return result;
}

void filc_native_zsys_setutxent(filc_thread* my_thread)
{
    PAS_UNUSED_PARAM(my_thread);
    pas_lock_lock(&utmpx_lock);
    setutxent();
    pas_lock_unlock(&utmpx_lock);
}

struct musl_lastlogx {
    struct musl_timeval ll_tv;
    char ll_line[32];
    char ll_host[256];
};

static void to_musl_lastlogx(struct lastlogx* lastlogx, struct musl_lastlogx* musl_lastlogx)
{
    musl_lastlogx->ll_tv.tv_sec = lastlogx->ll_tv.tv_sec;
    musl_lastlogx->ll_tv.tv_usec = lastlogx->ll_tv.tv_usec;

    lastlogx->ll_line[sizeof(lastlogx->ll_line) - 1] = 0;
    lastlogx->ll_host[sizeof(lastlogx->ll_host) - 1] = 0;

    snprintf(musl_lastlogx->ll_line, sizeof(musl_lastlogx->ll_line), "%s", lastlogx->ll_line);
    snprintf(musl_lastlogx->ll_host, sizeof(musl_lastlogx->ll_host), "%s", lastlogx->ll_host);
}

static filc_ptr handle_lastlogx_result(filc_thread* my_thread,
                                       filc_ptr musl_lastlogx_ptr, struct lastlogx* result)
{
    if (!result) {
        set_errno(errno);
        return filc_ptr_forge_null();
    }
    
    if (filc_ptr_ptr(musl_lastlogx_ptr)) {
        to_musl_lastlogx(result, (struct musl_lastlogx*)filc_ptr_ptr(musl_lastlogx_ptr));
        return musl_lastlogx_ptr;
    }

    filc_ptr result_ptr = filc_ptr_create(
        my_thread, filc_allocate_int(my_thread, sizeof(struct musl_lastlogx)));
    struct musl_lastlogx* musl_result = (struct musl_lastlogx*)filc_ptr_ptr(result_ptr);
    to_musl_lastlogx(result, musl_result);
    return result_ptr;
}

filc_ptr filc_native_zsys_getlastlogx(filc_thread* my_thread, unsigned uid, filc_ptr musl_lastlogx_ptr)
{
    if (filc_ptr_ptr(musl_lastlogx_ptr))
        filc_check_access_int(musl_lastlogx_ptr, sizeof(struct musl_lastlogx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct lastlogx lastlogx;
    filc_ptr result = handle_lastlogx_result(my_thread, musl_lastlogx_ptr, getlastlogx(uid, &lastlogx));
    pas_lock_unlock(&utmpx_lock);
    return result;
}

filc_ptr filc_native_zsys_getlastlogxbyname(filc_thread* my_thread, filc_ptr name_ptr,
                                            filc_ptr musl_lastlogx_ptr)
{
    char* name = filc_check_and_get_new_str(name_ptr);
    if (filc_ptr_ptr(musl_lastlogx_ptr))
        filc_check_access_int(musl_lastlogx_ptr, sizeof(struct musl_lastlogx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct lastlogx lastlogx;
    filc_ptr result = handle_lastlogx_result(
        my_thread, musl_lastlogx_ptr, getlastlogxbyname(name, &lastlogx));
    pas_lock_unlock(&utmpx_lock);
    bmalloc_deallocate(name);
    return result;
}

struct musl_cmsghdr {
    unsigned cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

static void destroy_msghdr(struct msghdr* msghdr, struct musl_msghdr* musl_msghdr)
{
    unprepare_iovec(
        msghdr->msg_iov, filc_ptr_load_with_manual_tracking(&musl_msghdr->msg_iov),
        musl_msghdr->msg_iovlen);
    bmalloc_deallocate(msghdr->msg_name);
    bmalloc_deallocate(msghdr->msg_control);
}

static void from_musl_msghdr_base(filc_thread* my_thread,
                                  struct musl_msghdr* musl_msghdr, struct msghdr* msghdr)
{
    pas_zero_memory(msghdr, sizeof(struct msghdr));

    int iovlen = musl_msghdr->msg_iovlen;
    msghdr->msg_iov = prepare_iovec(my_thread, filc_ptr_load(my_thread, &musl_msghdr->msg_iov), iovlen);
    msghdr->msg_iovlen = iovlen;

    msghdr->msg_flags = 0; /* This field is ignored so just zero-init it. */
}

static bool from_musl_msghdr_for_send(filc_thread* my_thread,
                                      struct musl_msghdr* musl_msghdr, struct msghdr* msghdr)
{
    static const bool verbose = false;

    if (verbose)
        pas_log("In from_musl_msghdr_for_send\n");
    
    from_musl_msghdr_base(my_thread, musl_msghdr, msghdr);

    unsigned msg_namelen = musl_msghdr->msg_namelen;
    if (msg_namelen) {
        filc_ptr msg_name = filc_ptr_load(my_thread, &musl_msghdr->msg_name);
        filc_check_access_int(msg_name, msg_namelen, NULL);
        if (!from_musl_sockaddr((struct musl_sockaddr*)filc_ptr_ptr(msg_name), msg_namelen,
                                (struct sockaddr**)&msghdr->msg_name, &msghdr->msg_namelen))
            goto error;
    }

    unsigned musl_controllen = musl_msghdr->msg_controllen;
    if (musl_controllen) {
        filc_ptr musl_control = filc_ptr_load(my_thread, &musl_msghdr->msg_control);
        filc_check_access_int(musl_control, musl_controllen, NULL);
        unsigned offset = 0;
        size_t cmsg_total_size = 0;
        for (;;) {
            unsigned offset_to_end;
            FILC_ASSERT(!pas_add_uint32_overflow(offset, sizeof(struct musl_cmsghdr), &offset_to_end),
                        NULL);
            if (offset_to_end > musl_controllen)
                break;
            struct musl_cmsghdr* cmsg = (struct musl_cmsghdr*)((char*)filc_ptr_ptr(musl_control) + offset);
            unsigned cmsg_len = cmsg->cmsg_len;
            FILC_ASSERT(!pas_add_uint32_overflow(offset, cmsg_len, &offset_to_end), NULL);
            FILC_ASSERT(offset_to_end <= musl_controllen, NULL);
            FILC_ASSERT(cmsg_len >= sizeof(struct musl_cmsghdr), NULL);
            FILC_ASSERT(cmsg_len >= pas_round_up_to_power_of_2(sizeof(struct musl_cmsghdr), sizeof(size_t)),
                        NULL);
            unsigned payload_len = cmsg_len - pas_round_up_to_power_of_2(
                sizeof(struct musl_cmsghdr), sizeof(size_t));
            FILC_ASSERT(!pas_add_uintptr_overflow(
                            cmsg_total_size, CMSG_SPACE(payload_len), &cmsg_total_size),
                        NULL);
            offset = pas_round_up_to_power_of_2(offset_to_end, sizeof(long));
            FILC_ASSERT(offset >= offset_to_end, NULL);
            FILC_ASSERT(offset <= musl_controllen, NULL);
        }

        FILC_ASSERT((unsigned)cmsg_total_size == cmsg_total_size, NULL);

        if (verbose)
            pas_log("cmsg_total_size = %zu\n", cmsg_total_size);
        
        struct cmsghdr* control = bmalloc_allocate_zeroed(cmsg_total_size);
        pas_zero_memory(control, cmsg_total_size);
        msghdr->msg_control = control;
        msghdr->msg_controllen = (unsigned)cmsg_total_size;

        offset = 0;
        struct cmsghdr* cmsg = CMSG_FIRSTHDR(msghdr);
        PAS_ASSERT(cmsg);
        for (;;) {
            unsigned offset_to_end;
            FILC_ASSERT(!pas_add_uint32_overflow(offset, sizeof(struct musl_cmsghdr), &offset_to_end),
                        NULL);
            if (offset_to_end > musl_controllen)
                break;
            PAS_ASSERT(cmsg);
            struct musl_cmsghdr* musl_cmsg =
                (struct musl_cmsghdr*)((char*)filc_ptr_ptr(musl_control) + offset);
            if (verbose)
                pas_log("cmsg = %p, musl_cmsg = %p\n", cmsg, musl_cmsg);
            unsigned musl_cmsg_len = musl_cmsg->cmsg_len;
            /* Do all the checks again so we don't get TOCTOUd. */
            FILC_ASSERT(!pas_add_uint32_overflow(offset, musl_cmsg_len, &offset_to_end), NULL);
            FILC_ASSERT(offset_to_end <= musl_controllen, NULL);
            FILC_ASSERT(musl_cmsg_len >= sizeof(struct musl_cmsghdr), NULL);
            FILC_ASSERT(musl_cmsg_len >= pas_round_up_to_power_of_2(
                            sizeof(struct musl_cmsghdr), sizeof(size_t)),
                        NULL);
            unsigned payload_len = musl_cmsg_len - pas_round_up_to_power_of_2(
                sizeof(struct musl_cmsghdr), sizeof(size_t));
            if (verbose)
                pas_log("payload_len = %u\n", payload_len);
            offset = pas_round_up_to_power_of_2(offset_to_end, sizeof(long));
            FILC_ASSERT(offset >= offset_to_end, NULL);
            FILC_ASSERT(offset <= musl_controllen, NULL);
            switch (musl_cmsg->cmsg_type) {
            case 0x01:
                cmsg->cmsg_type = SCM_RIGHTS;
                break;
            default:
                /* We don't support any cmsg other than SCM_RIGHTS right now. */
                goto error;
            }
            if (!from_musl_socket_level(musl_cmsg->cmsg_level, &cmsg->cmsg_level))
                goto error;
            cmsg->cmsg_len = CMSG_LEN(payload_len);
            memcpy(CMSG_DATA(cmsg), musl_cmsg + 1, payload_len);
            if (verbose) {
                pas_log("sizeof(cmsghdr) = %zu\n", sizeof(struct cmsghdr));
                pas_log("cmsg->cmsg_type = %d\n", cmsg->cmsg_type);
                pas_log("cmsg->cmsg_level = %d\n", cmsg->cmsg_level);
                pas_log("cmsg->cmsg_len = %u\n", (unsigned)cmsg->cmsg_len);
                pas_log("cmsg data as int = %d\n", *(int*)CMSG_DATA(cmsg));
            }
            cmsg = CMSG_NXTHDR(msghdr, cmsg);
        }
    }

    if (verbose)
        pas_log("from_musl_msghdr_for_send succeeded\n");

    return true;
error:
    destroy_msghdr(msghdr, musl_msghdr);
    return false;
}

static bool from_musl_msghdr_for_recv(filc_thread* my_thread,
                                      struct musl_msghdr* musl_msghdr, struct msghdr* msghdr)
{
    from_musl_msghdr_base(my_thread, musl_msghdr, msghdr);

    unsigned musl_namelen = musl_msghdr->msg_namelen;
    if (musl_namelen) {
        filc_check_access_int(filc_ptr_load(my_thread, &musl_msghdr->msg_name), musl_namelen, NULL);
        msghdr->msg_namelen = MAX_SOCKADDRLEN;
        msghdr->msg_name = bmalloc_allocate_zeroed(MAX_SOCKADDRLEN);
    }

    unsigned musl_controllen = musl_msghdr->msg_controllen;
    if (musl_controllen) {
        /* This code relies on the fact that the amount of space that musl requires you to allocate for
           control headers is always greater than the amount that MacOS requires. */
        msghdr->msg_control = bmalloc_allocate_zeroed(musl_controllen);
        msghdr->msg_controllen = musl_controllen;
    }
    
    return true;
}

static void to_musl_msghdr_for_recv(filc_thread* my_thread,
                                    struct msghdr* msghdr, struct musl_msghdr* musl_msghdr)
{
    if (msghdr->msg_namelen) {
        unsigned musl_namelen = musl_msghdr->msg_namelen;
        filc_ptr musl_name_ptr = filc_ptr_load(my_thread, &musl_msghdr->msg_name);
        filc_check_access_int(musl_name_ptr, musl_namelen, NULL);
        struct musl_sockaddr* musl_name = (struct musl_sockaddr*)filc_ptr_ptr(musl_name_ptr);
        PAS_ASSERT(to_musl_sockaddr(my_thread, msghdr->msg_name, msghdr->msg_namelen,
                                    musl_name, &musl_namelen));
        musl_msghdr->msg_namelen = musl_namelen;
    }
    
    unsigned musl_controllen = musl_msghdr->msg_controllen;
    filc_ptr musl_control = filc_ptr_load(my_thread, &musl_msghdr->msg_control);
    filc_check_access_int(musl_control, musl_controllen, NULL);
    char* musl_control_raw = (char*)filc_ptr_ptr(musl_control);
    pas_zero_memory(musl_control_raw, musl_controllen);

    PAS_ASSERT(to_musl_msg_flags(msghdr->msg_flags, &musl_msghdr->msg_flags));

    unsigned controllen = msghdr->msg_controllen;
    if (controllen) {
        struct cmsghdr* cmsg;
        unsigned needed_musl_len = 0;
        for (cmsg = CMSG_FIRSTHDR(msghdr); cmsg; cmsg = CMSG_NXTHDR(msghdr, cmsg)) {
            char* cmsg_end = (char*)cmsg + cmsg->cmsg_len;
            char* cmsg_data = (char*)CMSG_DATA(cmsg);
            PAS_ASSERT(cmsg_end >= cmsg_data);
            unsigned payload_len = cmsg_end - cmsg_data;
            unsigned aligned_payload_len = pas_round_up_to_power_of_2(payload_len, sizeof(long));
            PAS_ASSERT(aligned_payload_len >= payload_len);
            struct musl_cmsghdr musl_cmsghdr;
            musl_cmsghdr.cmsg_len =
                pas_round_up_to_power_of_2(sizeof(struct musl_cmsghdr), sizeof(size_t)) + payload_len;
            PAS_ASSERT(to_musl_socket_level(cmsg->cmsg_level, &musl_cmsghdr.cmsg_level));
            switch (cmsg->cmsg_type) {
            case SCM_RIGHTS:
                musl_cmsghdr.cmsg_type = 0x01;
                break;
            default:
                PAS_ASSERT(!"Should not be reached");
                break;
            }
            if (needed_musl_len <= musl_controllen) {
                memcpy(musl_control_raw + needed_musl_len, &musl_cmsghdr,
                       pas_min_uint32(musl_controllen - needed_musl_len, sizeof(struct musl_cmsghdr)));
            }
            PAS_ASSERT(!pas_add_uint32_overflow(
                           needed_musl_len, sizeof(struct musl_cmsghdr), &needed_musl_len));
            if (needed_musl_len <= musl_controllen) {
                memcpy(musl_control_raw + needed_musl_len, cmsg_data,
                       pas_min_uint32(musl_controllen - needed_musl_len, payload_len));
            }
            PAS_ASSERT(!pas_add_uint32_overflow(
                           needed_musl_len, aligned_payload_len, &needed_musl_len));
        }
        if (needed_musl_len > musl_controllen)
            musl_msghdr->msg_flags |= 0x0008; /* MSG_CTRUNC */
    }
}

ssize_t filc_native_zsys_sendmsg(filc_thread* my_thread, int sockfd, filc_ptr msg_ptr, int musl_flags)
{
    static const bool verbose = false;
    
    if (verbose)
        pas_log("In sendmsg\n");

    check_musl_msghdr(msg_ptr);
    struct musl_msghdr* musl_msg = (struct musl_msghdr*)filc_ptr_ptr(msg_ptr);
    int flags;
    if (!from_musl_msg_flags(musl_flags, &flags))
        goto einval;
    struct msghdr msg;
    if (!from_musl_msghdr_for_send(my_thread, musl_msg, &msg))
        goto einval;
    if (verbose)
        pas_log("Got this far\n");
    filc_exit(my_thread);
    if (verbose)
        pas_log("Actually doing sendmsg\n");
    ssize_t result = sendmsg(sockfd, &msg, flags);
    int my_errno = errno;
    if (verbose)
        pas_log("sendmsg result = %ld\n", (long)result);
    filc_enter(my_thread);
    destroy_msghdr(&msg, musl_msg);
    if (result < 0)
        set_errno(errno);
    return result;

einval:
    set_errno(EINVAL);
    return -1;
}

ssize_t filc_native_zsys_recvmsg(filc_thread* my_thread, int sockfd, filc_ptr msg_ptr, int musl_flags)
{
    static const bool verbose = false;

    check_musl_msghdr(msg_ptr);
    struct musl_msghdr* musl_msg = (struct musl_msghdr*)filc_ptr_ptr(msg_ptr);
    int flags;
    if (verbose)
        pas_log("doing recvmsg\n");
    if (!from_musl_msg_flags(musl_flags, &flags)) {
        if (verbose)
            pas_log("Bad flags\n");
        goto einval;
    }
    struct msghdr msg;
    if (!from_musl_msghdr_for_recv(my_thread, musl_msg, &msg)) {
        if (verbose)
            pas_log("Bad msghdr\n");
        goto einval;
    }
    filc_exit(my_thread);
    if (verbose) {
        pas_log("Actually doing recvmsg\n");
        pas_log("msg.msg_iov = %p\n", msg.msg_iov);
        pas_log("msg.msg_iovlen = %d\n", msg.msg_iovlen);
        int index;
        for (index = 0; index < msg.msg_iovlen; ++index)
            pas_log("msg.msg_iov[%d].iov_len = %zu\n", index, msg.msg_iov[index].iov_len);
    }
    ssize_t result = recvmsg(sockfd, &msg, flags);
    int my_errno = errno;
    if (verbose)
        pas_log("recvmsg result = %ld\n", (long)result);
    filc_enter(my_thread);
    if (result < 0) {
        if (verbose)
            pas_log("recvmsg failed: %s\n", strerror(errno));
        set_errno(my_errno);
    } else
        to_musl_msghdr_for_recv(my_thread, &msg, musl_msg);
    destroy_msghdr(&msg, musl_msg);
    return result;

einval:
    set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_fchmod(filc_thread* my_thread, int fd, unsigned mode)
{
    filc_exit(my_thread);
    int result = fchmod(fd, mode);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_rename(filc_thread* my_thread, filc_ptr oldname_ptr, filc_ptr newname_ptr)
{
    char* oldname = filc_check_and_get_new_str(oldname_ptr);
    char* newname = filc_check_and_get_new_str(newname_ptr);
    filc_exit(my_thread);
    int result = rename(oldname, newname);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(oldname);
    bmalloc_deallocate(newname);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_unlink(filc_thread* my_thread, filc_ptr path_ptr)
{
    char* path = filc_check_and_get_new_str(path_ptr);
    filc_exit(my_thread);
    int result = unlink(path);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(path);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_link(filc_thread* my_thread, filc_ptr oldname_ptr, filc_ptr newname_ptr)
{
    char* oldname = filc_check_and_get_new_str(oldname_ptr);
    char* newname = filc_check_and_get_new_str(newname_ptr);
    filc_exit(my_thread);
    int result = link(oldname, newname);
    int my_errno = errno;
    filc_enter(my_thread);
    bmalloc_deallocate(oldname);
    bmalloc_deallocate(newname);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

long filc_native_zsys_sysconf_override(filc_thread* my_thread, int musl_name)
{
    int name;
    switch (musl_name) {
    case 0:
        name = _SC_ARG_MAX;
        break;
    case 1:
        name = _SC_CHILD_MAX;
        break;
    case 2:
        name = _SC_CLK_TCK;
        break;
    case 3:
        name = _SC_NGROUPS_MAX;
        break;
    case 4:
        name = _SC_OPEN_MAX;
        break;
    case 5:
        name = _SC_STREAM_MAX;
        break;
    case 6:
        name = _SC_TZNAME_MAX;
        break;
    case 7:
        name = _SC_JOB_CONTROL;
        break;
    case 8:
        name = _SC_SAVED_IDS;
        break;
    case 9:
        name = _SC_REALTIME_SIGNALS;
        break;
    case 10:
        name = _SC_PRIORITY_SCHEDULING;
        break;
    case 11:
        name = _SC_TIMERS;
        break;
    case 12:
        name = _SC_ASYNCHRONOUS_IO;
        break;
    case 13:
        name = _SC_PRIORITIZED_IO;
        break;
    case 14:
        name = _SC_SYNCHRONIZED_IO;
        break;
    case 15:
        name = _SC_FSYNC;
        break;
    case 16:
        name = _SC_MAPPED_FILES;
        break;
    case 17:
        name = _SC_MEMLOCK;
        break;
    case 18:
        name = _SC_MEMLOCK_RANGE;
        break;
    case 19:
        name = _SC_MEMORY_PROTECTION;
        break;
    case 20:
        name = _SC_MESSAGE_PASSING;
        break;
    case 21:
        name = _SC_SEMAPHORES;
        break;
    case 22:
        name = _SC_SHARED_MEMORY_OBJECTS;
        break;
    case 23:
        name = _SC_AIO_LISTIO_MAX;
        break;
    case 24:
        name = _SC_AIO_MAX;
        break;
    case 25:
        name = _SC_AIO_PRIO_DELTA_MAX;
        break;
    case 26:
        name = _SC_DELAYTIMER_MAX;
        break;
    case 27:
        name = _SC_MQ_OPEN_MAX;
        break;
    case 28:
        name = _SC_MQ_PRIO_MAX;
        break;
    case 29:
        name = _SC_VERSION;
        break;
    case 30:
        name = _SC_PAGESIZE;
        break;
    case 31:
        name = _SC_RTSIG_MAX;
        break;
    case 32:
        name = _SC_SEM_NSEMS_MAX;
        break;
    case 33:
        name = _SC_SEM_VALUE_MAX;
        break;
    case 34:
        name = _SC_SIGQUEUE_MAX;
        break;
    case 35:
        name = _SC_TIMER_MAX;
        break;
    case 36:
        name = _SC_BC_BASE_MAX;
        break;
    case 37:
        name = _SC_BC_DIM_MAX;
        break;
    case 38:
        name = _SC_BC_SCALE_MAX;
        break;
    case 39:
        name = _SC_BC_STRING_MAX;
        break;
    case 40:
        name = _SC_COLL_WEIGHTS_MAX;
        break;
    case 42:
        name = _SC_EXPR_NEST_MAX;
        break;
    case 43:
        name = _SC_LINE_MAX;
        break;
    case 44:
        name = _SC_RE_DUP_MAX;
        break;
    case 46:
        name = _SC_2_VERSION;
        break;
    case 47:
        name = _SC_2_C_BIND;
        break;
    case 48:
        name = _SC_2_C_DEV;
        break;
    case 49:
        name = _SC_2_FORT_DEV;
        break;
    case 50:
        name = _SC_2_FORT_RUN;
        break;
    case 51:
        name = _SC_2_SW_DEV;
        break;
    case 52:
        name = _SC_2_LOCALEDEF;
        break;
    case 60:
        name = _SC_IOV_MAX;
        break;
    case 67:
        name = _SC_THREADS;
        break;
    case 68:
        name = _SC_THREAD_SAFE_FUNCTIONS;
        break;
    case 69:
        name = _SC_GETGR_R_SIZE_MAX;
        break;
    case 70:
        name = _SC_GETPW_R_SIZE_MAX;
        break;
    case 71:
        name = _SC_LOGIN_NAME_MAX;
        break;
    case 72:
        name = _SC_TTY_NAME_MAX;
        break;
    case 83:
        name = _SC_NPROCESSORS_CONF;
        break;
    case 84:
        name = _SC_NPROCESSORS_ONLN;
        break;
    case 85:
        name = _SC_PHYS_PAGES;
        break;
    case 87:
        name = _SC_PASS_MAX;
        break;
    case 89:
        name = _SC_XOPEN_VERSION;
        break;
    case 90:
        name = _SC_XOPEN_XCU_VERSION;
        break;
    case 91:
        name = _SC_XOPEN_UNIX;
        break;
    case 92:
        name = _SC_XOPEN_CRYPT;
        break;
    case 93:
        name = _SC_XOPEN_ENH_I18N;
        break;
    case 94:
        name = _SC_XOPEN_SHM;
        break;
    case 95:
        name = _SC_2_CHAR_TERM;
        break;
    case 97:
        name = _SC_2_UPE;
        break;
    default:
        set_errno(ENOSYS);
        return -1;
    }
    filc_exit(my_thread);
    errno = 0;
    long result = sysconf(name);
    int my_errno = errno;
    filc_enter(my_thread);
    if (my_errno)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_numcores(filc_thread* my_thread)
{
    filc_exit(my_thread);
    unsigned result;
    size_t length = sizeof(result);
    int name[] = {
        CTL_HW,
        HW_AVAILCPU
    };
    int sysctl_result = sysctl(name, sizeof(name) / sizeof(int), &result, &length, 0, 0);
    int my_errno = errno;
    filc_enter(my_thread);
    if (sysctl_result < 0) {
        set_errno(my_errno);
        return -1;
    }
    PAS_ASSERT((int)result >= 0);
    return (int)result;
}

static bool from_musl_prot(int musl_prot, int* prot)
{
    *prot = 0;
    if (check_and_clear(&musl_prot, 1))
        *prot |= PROT_READ;
    if (check_and_clear(&musl_prot, 2))
        *prot |= PROT_WRITE;
    return !musl_prot;
}

static bool from_musl_mmap_flags(int musl_flags, int* flags)
{
    *flags = 0;
    if (check_and_clear(&musl_flags, 0x01))
        *flags |= MAP_SHARED;
    if (check_and_clear(&musl_flags, 0x02))
        *flags |= MAP_PRIVATE;
    if (check_and_clear(&musl_flags, 0x20))
        *flags |= MAP_ANON;
    return !musl_flags;
}

static filc_ptr mmap_error_result(void)
{
    return filc_ptr_forge_invalid((void*)(intptr_t)-1);
}

filc_ptr filc_native_zsys_mmap(filc_thread* my_thread, filc_ptr address, size_t length, int musl_prot,
                               int musl_flags, int fd, long offset)
{
    static const bool verbose = false;
    if (filc_ptr_ptr(address)) {
        set_errno(EINVAL);
        return mmap_error_result();
    }
    int prot;
    if (!from_musl_prot(musl_prot, &prot)) {
        set_errno(EINVAL);
        return mmap_error_result();
    }
    int flags;
    if (!from_musl_mmap_flags(musl_flags, &flags)) {
        set_errno(EINVAL);
        return mmap_error_result();
    }
    if (!(flags & MAP_SHARED) && !(flags & MAP_PRIVATE)) {
        set_errno(EINVAL);
        return mmap_error_result();
    }
    if (!!(flags & MAP_SHARED) && !!(flags & MAP_PRIVATE)) {
        set_errno(EINVAL);
        return mmap_error_result();
    }
    filc_exit(my_thread);
    void* raw_result = mmap(NULL, length, prot, flags, fd, offset);
    int my_errno = errno;
    filc_enter(my_thread);
    if (raw_result == (void*)(intptr_t)-1) {
        set_errno(my_errno);
        return mmap_error_result();
    }
    PAS_ASSERT(raw_result);
    filc_word_type initial_word_type;
    if ((flags & MAP_PRIVATE) && (flags & MAP_ANON) && fd == -1 && !offset) {
        if (verbose)
            pas_log("using unset word type.\n");
        initial_word_type = FILC_WORD_TYPE_UNSET;
    } else {
        if (verbose)
            pas_log("using int word type.\n");
        initial_word_type = FILC_WORD_TYPE_INT;
    }
    filc_object* object = filc_allocate_with_existing_data(
        my_thread, raw_result, length, FILC_OBJECT_FLAG_MMAP, initial_word_type);
    PAS_ASSERT(object->lower == raw_result);
    return filc_ptr_create_with_manual_tracking(object);
}

int filc_native_zsys_munmap(filc_thread* my_thread, filc_ptr address, size_t length)
{
    filc_object* object = object_for_deallocate(address);
    FILC_CHECK(
        filc_object_size(object) == length,
        NULL,
        "cannot partially munmap (ptr = %s, length = %zu).",
        filc_ptr_to_new_string(address), length);
    FILC_CHECK(
        object->flags & FILC_OBJECT_FLAG_MMAP,
        NULL,
        "cannot munmap something that was not mmapped (ptr = %s).",
        filc_ptr_to_new_string(address));
    filc_free_yolo(my_thread, object);
    filc_exit(my_thread);
    filc_soft_handshake(filc_soft_handshake_no_op_callback, NULL);
    int result = munmap(filc_ptr_ptr(address), length);
    int my_errno = errno;
    filc_enter(my_thread);
    PAS_ASSERT(!result || result == -1);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_ftruncate(filc_thread* my_thread, int fd, long length)
{
    filc_exit(my_thread);
    int result = ftruncate(fd, length);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        set_errno(my_errno);
    return result;
}

int filc_native_zsys_getentropy(filc_thread* my_thread, filc_ptr buf_ptr, size_t len)
{
    filc_check_access_int(buf_ptr, len, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    int result = getentropy(filc_ptr_ptr(buf_ptr), len);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    if (result < 0)
        set_errno(my_errno);
    return result;
}

filc_ptr filc_native_zsys_getcwd(filc_thread* my_thread, filc_ptr buf_ptr, size_t size)
{
    filc_check_access_int(buf_ptr, size, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    char* result = getcwd((char*)filc_ptr_ptr(buf_ptr), size);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    PAS_ASSERT(!result || result == (char*)filc_ptr_ptr(buf_ptr));
    if (!result)
        set_errno(my_errno);
    if (!result)
        return filc_ptr_forge_null();
    return buf_ptr;
}

filc_ptr filc_native_zthread_self(filc_thread* my_thread)
{
    static const bool verbose = false;
    filc_ptr result = filc_ptr_for_special_payload_with_manual_tracking(my_thread);
    if (verbose)
        pas_log("my_thread = %p, zthread_self result = %s\n", my_thread, filc_ptr_to_new_string(result));
    return result;
}

unsigned filc_native_zthread_get_id(filc_thread* my_thread, filc_ptr thread_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    check_zthread(thread_ptr);
    filc_thread* thread = (filc_thread*)filc_ptr_ptr(thread_ptr);
    return thread->tid;
}

unsigned filc_native_zthread_self_id(filc_thread* my_thread)
{
    PAS_ASSERT(my_thread->tid);
    return my_thread->tid;
}

filc_ptr filc_native_zthread_get_cookie(filc_thread* my_thread, filc_ptr thread_ptr)
{
    PAS_UNUSED_PARAM(my_thread);
    check_zthread(thread_ptr);
    filc_thread* thread = (filc_thread*)filc_ptr_ptr(thread_ptr);
    return filc_ptr_load_with_manual_tracking(&thread->cookie_ptr);
}

void filc_native_zthread_set_self_cookie(filc_thread* my_thread, filc_ptr cookie_ptr)
{
    filc_ptr_store(my_thread, &my_thread->cookie_ptr, cookie_ptr);
}

static void* start_thread(void* arg)
{
    static const bool verbose = false;
    
    filc_thread* thread;

    thread = (filc_thread*)arg;

    if (verbose)
        pas_log("thread %u starting\n", thread->tid);

    PAS_ASSERT(thread->has_started);
    PAS_ASSERT(!thread->has_stopped);
    PAS_ASSERT(!thread->error_starting);

    pthread_detach(thread->thread);

    PAS_ASSERT(!pthread_setspecific(filc_thread_key, thread));
    PAS_ASSERT(!thread->thread);
    pas_fence();
    thread->thread = pthread_self();
    
    filc_enter(thread);

    thread->tlc_node = verse_heap_get_thread_local_cache_node();
    thread->tlc_node_version = pas_thread_local_cache_node_version(thread->tlc_node);
    
    static const filc_origin origin = {
        .filename = "<runtime>",
        .function = "start_thread",
        .line = 0,
        .column = 0
    };

    struct {
        FILC_FRAME_BODY;
    } actual_frame;
    pas_zero_memory(&actual_frame, sizeof(actual_frame));
    filc_frame* frame = (filc_frame*)&actual_frame;
    frame->origin = &origin;
    filc_push_frame(thread, frame);

    filc_native_frame native_frame;
    filc_push_native_frame(thread, &native_frame);

    filc_return_buffer return_buffer;
    filc_ptr rets = filc_ptr_for_ptr_return_buffer(&return_buffer);

    filc_ptr args = filc_ptr_create(thread, filc_allocate(thread, sizeof(filc_ptr)));
    filc_check_access_ptr(args, NULL);
    filc_ptr_store(thread, (filc_ptr*)filc_ptr_ptr(args), filc_ptr_load(thread, &thread->arg_ptr));
    filc_ptr_store(thread, &thread->arg_ptr, filc_ptr_forge_null());

    filc_lock_top_native_frame(thread);

    if (verbose)
        pas_log("thread %u calling main function\n", thread->tid);

    thread->thread_main(thread, args, rets);

    if (verbose)
        pas_log("thread %u main function returned\n", thread->tid);

    filc_unlock_top_native_frame(thread);
    filc_ptr result = *(filc_ptr*)filc_ptr_ptr(rets);
    filc_thread_track_object(thread, filc_ptr_object(result));

    pas_system_mutex_lock(&thread->lock);
    PAS_ASSERT(!thread->has_stopped);
    PAS_ASSERT(thread->thread);
    PAS_ASSERT(thread->thread == pthread_self());
    filc_ptr_store(thread, &thread->result_ptr, result);
    pas_system_mutex_unlock(&thread->lock);

    filc_pop_native_frame(thread, &native_frame);
    filc_pop_frame(thread, frame);

    sigset_t set;
    pas_reasonably_fill_sigset(&set);
    if (verbose)
        pas_log("%s: blocking signals\n", __PRETTY_FUNCTION__);
    PAS_ASSERT(!pthread_sigmask(SIG_SETMASK, &set, NULL));

    fugc_donate(&thread->mark_stack);
    filc_thread_stop_allocators(thread);
    filc_thread_relinquish_tid(thread);
    PAS_ASSERT(!thread->mark_stack.num_objects);
    filc_object_array_destruct(&thread->mark_stack);
    pas_thread_local_cache_destroy(pas_lock_is_not_held);
    thread->is_stopping = true;
    filc_exit(thread);

    pas_system_mutex_lock(&thread->lock);
    PAS_ASSERT(!thread->has_stopped);
    PAS_ASSERT(thread->thread);
    PAS_ASSERT(!(thread->state & FILC_THREAD_STATE_ENTERED));
    PAS_ASSERT(!(thread->state & FILC_THREAD_STATE_DEFERRED_SIGNAL));
    size_t index;
    for (index = FILC_MAX_MUSL_SIGNUM + 1; index--;)
        PAS_ASSERT(!thread->num_deferred_signals[index]);
    thread->thread = NULL;
    thread->has_stopped = true;
    pas_system_condition_broadcast(&thread->cond);
    pas_system_mutex_unlock(&thread->lock);
    
    if (verbose)
        pas_log("thread %u disposing\n", thread->tid);

    filc_thread_dispose(thread);

    /* At this point, the GC no longer sees this thread except if the user is holding references to it.
       And since we're exited, the GC could run at any time. So the thread might be alive or it might be
       dead - we don't know. */

    return NULL;
}

filc_ptr filc_native_zthread_create(filc_thread* my_thread, filc_ptr callback_ptr, filc_ptr arg_ptr)
{
    filc_check_function_call(callback_ptr);
    filc_thread* thread = filc_thread_create();
    pas_system_mutex_lock(&thread->lock);
    /* I don't see how this could ever happen. */
    PAS_ASSERT(!thread->thread);
    PAS_ASSERT(filc_ptr_is_totally_null(thread->arg_ptr));
    PAS_ASSERT(filc_ptr_is_totally_null(thread->result_ptr));
    PAS_ASSERT(filc_ptr_is_totally_null(thread->cookie_ptr));
    thread->thread_main = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(callback_ptr);
    filc_ptr_store(my_thread, &thread->arg_ptr, arg_ptr);
    pas_system_mutex_unlock(&thread->lock);
    filc_exit(my_thread);
    /* Make sure we don't create threads while in a handshake. This will hold the thread in the
       !has_started && !thread state, so if the soft handshake doesn't see it, that's fine. */
    filc_stop_the_world_lock_lock();
    filc_soft_handshake_lock_lock();
    thread->has_started = true;
    pthread_t ignored_thread;
    int result = pthread_create(&ignored_thread, NULL, start_thread, thread);
    if (result)
        thread->has_started = false;
    filc_soft_handshake_lock_unlock();
    filc_stop_the_world_lock_unlock();
    filc_enter(my_thread);
    if (result) {
        pas_system_mutex_lock(&thread->lock);
        PAS_ASSERT(!thread->thread);
        thread->error_starting = true;
        pas_system_mutex_unlock(&thread->lock);
        filc_thread_dispose(thread);
        set_errno(result);
        return filc_ptr_forge_null();
    }
    return filc_ptr_for_special_payload_with_manual_tracking(thread);
}

bool filc_native_zthread_join(filc_thread* my_thread, filc_ptr thread_ptr, filc_ptr result_ptr)
{
    check_zthread(thread_ptr);
    if (filc_ptr_ptr(result_ptr))
        filc_check_access_ptr(result_ptr, NULL);
    filc_thread* thread = (filc_thread*)filc_ptr_ptr(thread_ptr);
    /* Should never happen because we'd never vend such a thread to the user. */
    PAS_ASSERT(thread->has_started);
    PAS_ASSERT(!thread->error_starting);
    if (thread->forked) {
        set_errno(ESRCH);
        return false;
    }
    filc_exit(my_thread);
    pas_system_mutex_lock(&thread->lock);
    /* Note that this loop doesn't have to worry about forked. If we forked and this ended up in a child,
       then this thread would be dead and we wouldn't care. */
    while (!thread->has_stopped)
        pas_system_condition_wait(&thread->cond, &thread->lock);
    pas_system_mutex_unlock(&thread->lock);
    filc_enter(my_thread);
    if (filc_ptr_ptr(result_ptr)) {
        filc_ptr_store(
            my_thread, (filc_ptr*)filc_ptr_ptr(result_ptr),
            filc_ptr_load_with_manual_tracking(&thread->result_ptr));
    }
    return true;
}

typedef struct {
    filc_thread* my_thread;
    void (*condition)(PIZLONATED_SIGNATURE);
    void (*before_sleep)(PIZLONATED_SIGNATURE);
    filc_ptr arg_ptr;
} zpark_if_data;

static bool zpark_if_validate_callback(void* arg)
{
    zpark_if_data* data = (zpark_if_data*)arg;

    filc_return_buffer return_buffer;
    filc_ptr rets = filc_ptr_for_int_return_buffer(&return_buffer);

    filc_ptr args = filc_ptr_create(
        data->my_thread, filc_allocate(data->my_thread, sizeof(filc_ptr)));
    filc_check_access_ptr(args, NULL);
    filc_ptr_store(data->my_thread, (filc_ptr*)filc_ptr_ptr(args), data->arg_ptr);

    filc_lock_top_native_frame(data->my_thread);
    data->condition(data->my_thread, args, rets);
    filc_unlock_top_native_frame(data->my_thread);

    return *(bool*)filc_ptr_ptr(rets);
}

static void zpark_if_before_sleep_callback(void* arg)
{
    zpark_if_data* data = (zpark_if_data*)arg;

    filc_return_buffer return_buffer;
    filc_ptr rets = filc_ptr_for_int_return_buffer(&return_buffer);

    filc_ptr args = filc_ptr_create(
        data->my_thread, filc_allocate(data->my_thread, sizeof(filc_ptr)));
    filc_check_access_ptr(args, NULL);
    filc_ptr_store(data->my_thread, (filc_ptr*)filc_ptr_ptr(args), data->arg_ptr);

    filc_lock_top_native_frame(data->my_thread);
    data->before_sleep(data->my_thread, args, rets);
    filc_unlock_top_native_frame(data->my_thread);
}

bool filc_native_zpark_if(filc_thread* my_thread, filc_ptr address_ptr, filc_ptr condition_ptr,
                          filc_ptr before_sleep_ptr, filc_ptr arg_ptr,
                          double absolute_timeout_in_milliseconds)
{
    FILC_CHECK(
        filc_ptr_ptr(address_ptr),
        NULL,
        "cannot zpark on a null address.");
    filc_check_function_call(condition_ptr);
    filc_check_function_call(before_sleep_ptr);
    zpark_if_data data;
    data.my_thread = my_thread;
    data.condition = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(condition_ptr);
    data.before_sleep = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(before_sleep_ptr);
    data.arg_ptr = arg_ptr;
    return filc_park_conditionally(my_thread,
                                   filc_ptr_ptr(address_ptr),
                                   zpark_if_validate_callback,
                                   zpark_if_before_sleep_callback,
                                   &data,
                                   absolute_timeout_in_milliseconds);
}

typedef struct {
    filc_thread* my_thread;
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

    filc_return_buffer return_buffer;
    filc_ptr rets = filc_ptr_for_int_return_buffer(&return_buffer);

    filc_ptr args = filc_ptr_create(
        data->my_thread, filc_allocate(data->my_thread, sizeof(zunpark_one_callback_args)));
    FILC_CHECK_INT_FIELD(args, zunpark_one_callback_args, did_unpark_thread);
    FILC_CHECK_INT_FIELD(args, zunpark_one_callback_args, may_have_more_threads);
    FILC_CHECK_PTR_FIELD(args, zunpark_one_callback_args, arg_ptr);
    zunpark_one_callback_args* raw_args = (zunpark_one_callback_args*)filc_ptr_ptr(args);
    raw_args->did_unpark_thread = result.did_unpark_thread;
    raw_args->may_have_more_threads = result.may_have_more_threads;
    filc_ptr_store(data->my_thread, &raw_args->arg_ptr, data->arg_ptr);

    filc_lock_top_native_frame(data->my_thread);
    data->callback(data->my_thread, args, rets);
    filc_unlock_top_native_frame(data->my_thread);
}

void filc_native_zunpark_one(filc_thread* my_thread, filc_ptr address_ptr, filc_ptr callback_ptr,
                             filc_ptr arg_ptr)
{
    FILC_CHECK(
        filc_ptr_ptr(address_ptr),
        NULL,
        "cannot zunpark on a null address.");
    filc_check_function_call(callback_ptr);
    zunpark_one_data data;
    data.my_thread = my_thread;
    data.callback = (void (*)(PIZLONATED_SIGNATURE))filc_ptr_ptr(callback_ptr);
    data.arg_ptr = arg_ptr;
    filc_unpark_one(my_thread, filc_ptr_ptr(address_ptr), zunpark_one_callback, &data);
}

unsigned filc_native_zunpark(filc_thread* my_thread, filc_ptr address_ptr, unsigned count)
{
    FILC_CHECK(
        filc_ptr_ptr(address_ptr),
        NULL,
        "cannot zunpark on a null address.");
    return filc_unpark(my_thread, filc_ptr_ptr(address_ptr), count);
}

bool filc_get_bool_env(const char* name, bool default_value)
{
    char* value = getenv(name);
    if (!value)
        return default_value;
    if (!strcmp(value, "1") ||
        !strcasecmp(value, "yes") ||
        !strcasecmp(value, "true"))
        return true;
    if (!strcmp(value, "0") ||
        !strcasecmp(value, "no") ||
        !strcasecmp(value, "false"))
        return false;
    pas_panic("invalid fugc environment variable %s value: %s (expected boolean like 1, yes, true, 0, no, "
              "or false)\n", name, value);
    return false;
}

unsigned filc_get_unsigned_env(const char* name, unsigned default_value)
{
    char* value = getenv(name);
    if (!value)
        return default_value;
    unsigned result;
    if (sscanf(value, "%u", &result) == 1)
        return result;
    pas_panic("invalid fugc environment variable %s value: %s (expected decimal unsigned int)\n",
              name, value);
    return 0;
}

size_t filc_get_size_env(const char* name, size_t default_value)
{
    char* value = getenv(name);
    if (!value)
        return default_value;
    size_t result;
    if (sscanf(value, "%zu", &result) == 1)
        return result;
    pas_panic("invalid fugc environment variable %s value: %s (expected decimal byte size)\n",
              name, value);
    return 0;
}

#endif /* PAS_ENABLE_FILC */

#endif /* LIBPAS_ENABLED */

