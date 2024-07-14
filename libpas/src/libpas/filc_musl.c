/*
 * Copyright (c) 2023-2024 Epic Games, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "pas_config.h"

#if LIBPAS_ENABLED

#include "filc_runtime.h"

#if PAS_ENABLE_FILC && FILC_MUSL

#include "bmalloc_heap.h"
#include "filc_native.h"
#include "pas_bitvector.h"
#include "pas_uint64_hash_map.h"
#include "pas_utils.h"
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
#include <grp.h>
#include <poll.h>
#include <spawn.h>

#if PAS_OS(LINUX)
#include <pty.h>
#else /* PAS_OS(LINUX) -> so !PAS_OS(LINUX) */
#include <sys/sysctl.h>
#endif /* PAS_OS(LINUX) -> so end of !PAS_OS(LINUX) */

#if PAS_OS(DARWIN) || PAS_OS(OPENBSD)
#include <util.h>
#endif /* PAS_OS(DARWIN) */

#if PAS_OS(FREEBSD)
#include <libutil.h>
#endif /* PAS_OS(FREEBSD) */

#if !PAS_OS(OPENBSD)
#include <utmpx.h>
#include <sys/random.h>
#define HAVE_UTMPX 1
#else /* !PAS_OS(OPENBSD) -> so PAS_OS(OPENBSD) */
#define HAVE_UTMPX 0
#endif /* !PAS_OS(OPENBSD) -> so end of PAS_OS(OPENBSD) */

#if PAS_OS(DARWIN)
#define HAVE_LASTLOGX 1
#else /* PAS_OS(DARWIN) -> so !PAS_OS(DARWIN) */
#define HAVE_LASTLOGX 0
#endif /* PAS_OS(DARWIN) -> so end of !PAS_OS(DARWIN) */

void filc_mark_user_global_roots(filc_object_array* mark_stack)
{
    PAS_UNUSED_PARAM(mark_stack);
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

static void check_musl_passwd(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_name, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_passwd, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_passwd, pw_uid, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_passwd, pw_gid, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_gecos, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_dir, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_passwd, pw_shell, access_kind);
}

struct musl_group {
    filc_ptr gr_name;
    filc_ptr gr_passwd;
    unsigned gr_gid;
    filc_ptr gr_mem;
};

static void check_musl_group(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_group, gr_name, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_group, gr_passwd, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_group, gr_gid, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_group, gr_mem, access_kind);
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

static void check_musl_addrinfo(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_flags, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_family, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_socktype, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_protocol, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_addrinfo, ai_addrlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_addrinfo, ai_addr, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_addrinfo, ai_canonname, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_addrinfo, ai_next, access_kind);
}

struct musl_dirent {
    uint64_t d_ino;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

static void check_musl_dirent(filc_ptr ptr, filc_access_kind access_kind)
{
    filc_check_access_int(ptr, sizeof(struct musl_dirent), access_kind, NULL);
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

int filc_to_user_errno(int errno_value)
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
#if PAS_OS(DARWIN)
    case ENOSTR         : return 60;
    case ENODATA        : return 61;
    case ETIME          : return 62;
    case ENOSR          : return 63;
#endif /* PAS_OS(DARWIN) */
    case EREMOTE        : return 66;
#if !PAS_OS(OPENBSD)
    case ENOLINK        : return 67;
#endif /* !PAS_OS(OPENBSD) */
    case EPROTO         : return 71;
#if !PAS_OS(OPENBSD)
    case EMULTIHOP      : return 72;
#endif /* !PAS_OS(OPENBSD) */
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
#if PAS_OS(DARWIN)
    case ENOTSUP        : return 95;
#endif /* PAS_OS(DARWIN) */
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

void filc_from_user_sigset(filc_user_sigset* user_sigset,
                           sigset_t* sigset)
{
    static const bool verbose = false;
    
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    PAS_ASSERT(!sigemptyset(sigset));
    
    unsigned musl_sigindex;
    for (musl_sigindex = num_active_bits; musl_sigindex--;) {
        int musl_signum = musl_sigindex + 1;
        bool bit_value = !!(user_sigset->bits[PAS_BITVECTOR_WORD64_INDEX(musl_sigindex)]
                            & PAS_BITVECTOR_BIT_MASK64(musl_sigindex));
        if (verbose)
            pas_log("musl_signum %u: %s\n", musl_signum, bit_value ? "yes" : "no");
        if (!bit_value)
            continue;
        int signum = filc_from_user_signum(musl_signum);
        if (signum < 0) {
            if (verbose)
                pas_log("no conversion, skipping.\n");
            continue;
        }
        sigaddset(sigset, signum);
    }
    if (verbose) {
        for (int sig = 1; sig < 32; ++sig)
            pas_log("signal %d masked: %s\n", sig, sigismember(sigset, sig) ? "yes" : "no");
    }
}

void filc_to_user_sigset(sigset_t* sigset, filc_user_sigset* user_sigset)
{
    static const unsigned num_active_words = 2;
    static const unsigned num_active_bits = 2 * 64;

    memset(user_sigset, 0, sizeof(filc_user_sigset));
    
    unsigned musl_signum;
    for (musl_signum = num_active_bits; musl_signum--;) {
        int signum = filc_from_user_signum(musl_signum);
        if (signum < 0)
            continue;
        if (sigismember(sigset, signum)) {
            PAS_ASSERT(musl_signum);
            unsigned musl_sigindex = musl_signum - 1;
            user_sigset->bits[PAS_BITVECTOR_WORD64_INDEX(musl_sigindex)] |=
                PAS_BITVECTOR_BIT_MASK64(musl_sigindex);
        }
    }
}

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
    if (filc_check_and_clear(&iflag, IGNBRK))
        result |= 01;
    if (filc_check_and_clear(&iflag, BRKINT))
        result |= 02;
    if (filc_check_and_clear(&iflag, IGNPAR))
        result |= 04;
    if (filc_check_and_clear(&iflag, PARMRK))
        result |= 010;
    if (filc_check_and_clear(&iflag, INPCK))
        result |= 020;
    if (filc_check_and_clear(&iflag, ISTRIP))
        result |= 040;
    if (filc_check_and_clear(&iflag, INLCR))
        result |= 0100;
    if (filc_check_and_clear(&iflag, IGNCR))
        result |= 0200;
    if (filc_check_and_clear(&iflag, ICRNL))
        result |= 0400;
    if (filc_check_and_clear(&iflag, IXON))
        result |= 02000;
    if (filc_check_and_clear(&iflag, IXANY))
        result |= 04000;
    if (filc_check_and_clear(&iflag, IXOFF))
        result |= 010000;
    if (filc_check_and_clear(&iflag, IMAXBEL))
        result |= 020000;
#if !PAS_OS(OPENBSD)
    if (filc_check_and_clear(&iflag, IUTF8))
        result |= 040000;
#endif /* !PAS_OS(OPENBSD) */
    PAS_ASSERT(!iflag);
    return result;
}

static bool from_musl_ciflag(unsigned musl_iflag, tcflag_t* result)
{
    *result = 0;
    if (filc_check_and_clear(&musl_iflag, 01))
        *result |= IGNBRK;
    if (filc_check_and_clear(&musl_iflag, 02))
        *result |= BRKINT;
    if (filc_check_and_clear(&musl_iflag, 04))
        *result |= IGNPAR;
    if (filc_check_and_clear(&musl_iflag, 010))
        *result |= PARMRK;
    if (filc_check_and_clear(&musl_iflag, 020))
        *result |= INPCK;
    if (filc_check_and_clear(&musl_iflag, 040))
        *result |= ISTRIP;
    if (filc_check_and_clear(&musl_iflag, 0100))
        *result |= INLCR;
    if (filc_check_and_clear(&musl_iflag, 0200))
        *result |= IGNCR;
    if (filc_check_and_clear(&musl_iflag, 0400))
        *result |= ICRNL;
    if (filc_check_and_clear(&musl_iflag, 02000))
        *result |= IXON;
    if (filc_check_and_clear(&musl_iflag, 04000))
        *result |= IXANY;
    if (filc_check_and_clear(&musl_iflag, 010000))
        *result |= IXOFF;
    if (filc_check_and_clear(&musl_iflag, 020000))
        *result |= IMAXBEL;
#if !PAS_OS(OPENBSD)
    if (filc_check_and_clear(&musl_iflag, 040000))
        *result |= IUTF8;
#endif /* !PAS_OS(OPENBSD) */
    return !musl_iflag;
}

static unsigned to_musl_coflag(unsigned oflag)
{
    unsigned result = 0;
    if (filc_check_and_clear(&oflag, OPOST))
        result |= 01;
    if (filc_check_and_clear(&oflag, ONLCR))
        result |= 04;
#if !PAS_OS(LINUX)
    /* Maybe I should teach musl about OXTABS or pass it through? meh? */
    PAS_ASSERT(!(oflag & ~OXTABS));
#endif /* !PAS_OS(LINUX) */
    return result;
}

static bool from_musl_coflag(unsigned musl_oflag, tcflag_t* result)
{
    *result = 0;
    if (filc_check_and_clear(&musl_oflag, 01))
        *result |= OPOST;
    if (filc_check_and_clear(&musl_oflag, 04))
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
    if (filc_check_and_clear(&cflag, CSTOPB))
        result |= 0100;
    if (filc_check_and_clear(&cflag, CREAD))
        result |= 0200;
    if (filc_check_and_clear(&cflag, PARENB))
        result |= 0400;
    if (filc_check_and_clear(&cflag, PARODD))
        result |= 01000;
    if (filc_check_and_clear(&cflag, HUPCL))
        result |= 02000;
    if (filc_check_and_clear(&cflag, CLOCAL))
        result |= 04000;
#if PAS_OS(LINUX)
    cflag &= ~CBAUD;
#endif /* PAS_OS(LINUX) */
    if (cflag)
        pas_log("Unhandled cflag: 0x%x\n", cflag);
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
    if (filc_check_and_clear(&musl_cflag, 0100))
        *result |= CSTOPB;
    if (filc_check_and_clear(&musl_cflag, 0200))
        *result |= CREAD;
    if (filc_check_and_clear(&musl_cflag, 0400))
        *result |= PARENB;
    if (filc_check_and_clear(&musl_cflag, 01000))
        *result |= PARODD;
    if (filc_check_and_clear(&musl_cflag, 02000))
        *result |= HUPCL;
    if (filc_check_and_clear(&musl_cflag, 04000))
        *result |= CLOCAL;
    return !musl_cflag;
}

static unsigned to_musl_clflag(unsigned lflag)
{
    unsigned result = 0;
    if (filc_check_and_clear(&lflag, ISIG))
        result |= 01;
    if (filc_check_and_clear(&lflag, ICANON))
        result |= 02;
    if (filc_check_and_clear(&lflag, ECHO))
        result |= 010;
    if (filc_check_and_clear(&lflag, ECHOE))
        result |= 020;
    if (filc_check_and_clear(&lflag, ECHOK))
        result |= 040;
    if (filc_check_and_clear(&lflag, ECHONL))
        result |= 0100;
    if (filc_check_and_clear(&lflag, NOFLSH))
        result |= 0200;
    if (filc_check_and_clear(&lflag, TOSTOP))
        result |= 0400;
    if (filc_check_and_clear(&lflag, ECHOCTL))
        result |= 01000;
    if (filc_check_and_clear(&lflag, ECHOPRT))
        result |= 02000;
    if (filc_check_and_clear(&lflag, ECHOKE))
        result |= 04000;
    if (filc_check_and_clear(&lflag, FLUSHO))
        result |= 010000;
    if (filc_check_and_clear(&lflag, PENDIN))
        result |= 040000;
    if (filc_check_and_clear(&lflag, IEXTEN))
        result |= 0100000;
    if (filc_check_and_clear(&lflag, EXTPROC))
        result |= 0200000;
    PAS_ASSERT(!lflag);
    return result;
}

static bool from_musl_clflag(unsigned musl_lflag, tcflag_t* result)
{
    *result = 0;
    if (filc_check_and_clear(&musl_lflag, 01))
        *result |= ISIG;
    if (filc_check_and_clear(&musl_lflag, 02))
        *result |= ICANON;
    if (filc_check_and_clear(&musl_lflag, 010))
        *result |= ECHO;
    if (filc_check_and_clear(&musl_lflag, 020))
        *result |= ECHOE;
    if (filc_check_and_clear(&musl_lflag, 040))
        *result |= ECHOK;
    if (filc_check_and_clear(&musl_lflag, 0100))
        *result |= ECHONL;
    if (filc_check_and_clear(&musl_lflag, 0200))
        *result |= NOFLSH;
    if (filc_check_and_clear(&musl_lflag, 0400))
        *result |= TOSTOP;
    if (filc_check_and_clear(&musl_lflag, 01000))
        *result |= ECHOCTL;
    if (filc_check_and_clear(&musl_lflag, 02000))
        *result |= ECHOPRT;
    if (filc_check_and_clear(&musl_lflag, 04000))
        *result |= ECHOKE;
    if (filc_check_and_clear(&musl_lflag, 010000))
        *result |= FLUSHO;
    if (filc_check_and_clear(&musl_lflag, 040000))
        *result |= PENDIN;
    if (filc_check_and_clear(&musl_lflag, 0100000))
        *result |= IEXTEN;
    if (filc_check_and_clear(&musl_lflag, 0200000))
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

int filc_native_zsys_ioctl(filc_thread* my_thread, int fd, unsigned long request, filc_cc_cursor* args)
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
        filc_exit(my_thread);
        if (tcgetattr(fd, &termios) < 0)
            goto error;
        filc_enter(my_thread);
        musl_termios_ptr = filc_cc_cursor_get_next_ptr(args);
        filc_check_write_int(musl_termios_ptr, sizeof(struct musl_termios), NULL);
        musl_termios = (struct musl_termios*)filc_ptr_ptr(musl_termios_ptr);
        to_musl_termios(&termios, musl_termios);
        return 0;
    }
    case 0x5402: /* TCGETS */
    case 0x5403: /* TCGETSW */
    case 0x5404: /* TCGETSF */ {
        struct termios termios;
        struct musl_termios* musl_termios;
        filc_ptr musl_termios_ptr;
        musl_termios_ptr = filc_cc_cursor_get_next_ptr(args);
        filc_check_read_int(musl_termios_ptr, sizeof(struct musl_termios), NULL);
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
        filc_ptr arg_ptr = filc_cc_cursor_get_next_ptr(args);
        if (filc_ptr_ptr(arg_ptr))
            filc_check_read_int(arg_ptr, sizeof(int), NULL);
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
        filc_exit(my_thread);
        int result = ioctl(fd, TIOCGWINSZ, &winsize);
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        musl_winsize_ptr = filc_cc_cursor_get_next_ptr(args);
        filc_check_write_int(musl_winsize_ptr, sizeof(struct musl_winsize), NULL);
        musl_winsize = (struct musl_winsize*)filc_ptr_ptr(musl_winsize_ptr);
        to_musl_winsize(&winsize, musl_winsize);
        return result;
    }
    case 0x5414: /* TIOCSWINSZ */ {
        filc_ptr musl_winsize_ptr;
        struct musl_winsize* musl_winsize;
        struct winsize winsize;
        musl_winsize_ptr = filc_cc_cursor_get_next_ptr(args);
        filc_check_read_int(musl_winsize_ptr, sizeof(struct musl_winsize), NULL);
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
    case 0x5450: /* FIONCLEX */ {
        filc_exit(my_thread);
        int result = ioctl(fd, FIONCLEX, NULL);
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        return result;
    }
    case 0x5451: /* FIOCLEX */ {
        filc_exit(my_thread);
        int result = ioctl(fd, FIOCLEX, NULL);
        if (result < 0)
            goto error;
        filc_enter(my_thread);
        return result;
    }
    default:
        if (verbose)
            pas_log("Invalid ioctl request = %lu.\n", request);
        filc_set_errno(EINVAL);
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
    filc_set_errno(my_errno);
    return -1;
}

static struct filc_ptr to_musl_passwd(filc_thread* my_thread, struct passwd* passwd)
{
    filc_ptr result_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(struct musl_passwd)));
    check_musl_passwd(result_ptr, filc_write_access);

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
        filc_set_errno(errno);
        return filc_ptr_forge_null();
    }
    return to_musl_passwd(my_thread, passwd);
}

int filc_native_zsys_isatty(filc_thread* my_thread, int fd)
{
    filc_exit(my_thread);
    errno = 0;
    int result = isatty(fd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (!result && my_errno)
        filc_set_errno(my_errno);
    return result;
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
#if PAS_OS(DARWIN)
    case 40:
        *result = PF_VSOCK;
        return true;
#endif /* PAS_OS(DARWIN) */
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
#if PAS_OS(DARWIN)
    case PF_VSOCK:
        *result = 40;
        return true;
#endif /* PAS_OS(DARWIN) */
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
        filc_set_errno(EINVAL);
        return -1;
    }
    int type;
    if (!from_musl_socket_type(musl_type, &type)) {
        filc_set_errno(EINVAL);
        return -1;
    }
    filc_exit(my_thread);
    int result = socket(domain, type, protocol);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
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
#if PAS_OS(DARWIN)
    case 4:
        *result = TCP_KEEPALIVE; /* musl says KEEPIDLE, Darwin says KEEPALIVE. coooool. */
        return true;
#elif !PAS_OS(OPENBSD)
    case 4:
        *result = TCP_KEEPIDLE;
        return true;
#endif /* !PAS_OS(OPENBSD) */
#if !PAS_OS(OPENBSD)
    case 5:
        *result = TCP_KEEPINTVL;
        return true;
#endif /* !PAS_OS(OPENBSD) */
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
    filc_check_read_int(optval_ptr, optlen, NULL);
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
            if (optlen < sizeof(filc_user_timeval))
                goto einval;
            struct timeval* tv = alloca(sizeof(struct timeval));
            filc_user_timeval* musl_tv = (filc_user_timeval*)optval;
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
#if PAS_OS(DARWIN)
        case TCP_KEEPALIVE:
            break;
#elif !PAS_OS(OPENBSD)
        case TCP_KEEPIDLE:
            break;
#endif /* !PAS_OS(OPENBSD) */
#if !PAS_OS(OPENBSD)
        case TCP_KEEPINTVL:
            break;
#endif /* !PAS_OS(OPENBSD) */
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
        filc_set_errno(my_errno);
    return result;
    
enoprotoopt:
    filc_set_errno(ENOPROTOOPT);
    return -1;
    
einval:
    filc_set_errno(EINVAL);
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
    
    filc_check_read_int(musl_addr_ptr, musl_addrlen, NULL);
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
        filc_set_errno(my_errno);

    bmalloc_deallocate(addr);
    return result;
    
einval:
    filc_set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_connect(filc_thread* my_thread,
                             int sockfd, filc_ptr musl_addr_ptr, unsigned musl_addrlen)
{
    static const bool verbose = false;
    
    filc_check_read_int(musl_addr_ptr, musl_addrlen, NULL);
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
        filc_set_errno(my_errno);

    bmalloc_deallocate(addr);
    if (verbose)
        pas_log("connect succeeded!\n");
    return result;
    
einval:
    filc_set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_getsockname(filc_thread* my_thread,
                                 int sockfd, filc_ptr musl_addr_ptr, filc_ptr musl_addrlen_ptr)
{
    static const bool verbose = false;
    
    filc_check_read_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
    unsigned musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);

    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = (struct sockaddr*)alloca(addrlen);
    filc_exit(my_thread);
    int result = getsockname(sockfd, addr, &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        filc_set_errno(my_errno);
        return result;
    }
    
    PAS_ASSERT(addrlen <= MAX_SOCKADDRLEN);

    filc_check_write_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
    filc_check_write_int(musl_addr_ptr, musl_addrlen, NULL);
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
    
    filc_check_read_int(optlen_ptr, sizeof(unsigned), NULL);
    unsigned musl_optlen = *(unsigned*)filc_ptr_ptr(optlen_ptr);
    int level;
    if (verbose)
        pas_log("here!\n");
    if (!from_musl_socket_level(musl_level, &level))
        goto einval;
    int optname;
    /* everything is primitive, for now */
    filc_check_write_int(optval_ptr, musl_optlen, NULL);
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
            if (musl_optlen < sizeof(filc_user_timeval))
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
#if PAS_OS(DARWIN)
        case TCP_KEEPALIVE:
            break;
#elif !PAS_OS(OPENBSD)
        case TCP_KEEPIDLE:
            break;
#endif /* !PAS_OS(OPENBSD) */
#if !PAS_OS(OPENBSD)
        case TCP_KEEPINTVL:
            break;
#endif /* !PAS_OS(OPENBSD) */
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
        filc_set_errno(my_errno);
        return result;
    }
    musl_optlen = optlen;
    switch (level) {
    case SOL_SOCKET:
        switch (optname) {
        case SO_RCVTIMEO: {
            struct timeval* tv = (struct timeval*)optval;
            filc_user_timeval* musl_tv = (filc_user_timeval*)musl_optval;
            musl_tv->tv_sec = tv->tv_sec;
            musl_tv->tv_usec = tv->tv_usec;
            musl_optlen = sizeof(filc_user_timeval);
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
    filc_check_write_int(optlen_ptr, sizeof(unsigned), NULL);
    *(unsigned*)filc_ptr_ptr(optlen_ptr) = musl_optlen;
    return 0;

enoprotoopt:
    if (verbose)
        pas_log("bad proto\n");
    filc_set_errno(ENOPROTOOPT);
    return -1;
    
einval:
    if (verbose)
        pas_log("einval\n");
    filc_set_errno(EINVAL);
    return -1;
}

int filc_native_zsys_getpeername(filc_thread* my_thread, int sockfd, filc_ptr musl_addr_ptr,
                                 filc_ptr musl_addrlen_ptr)
{
    static const bool verbose = false;
    
    filc_check_read_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
    unsigned musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);

    unsigned addrlen = MAX_SOCKADDRLEN;
    struct sockaddr* addr = (struct sockaddr*)alloca(addrlen);
    filc_exit(my_thread);
    int result = getpeername(sockfd, addr, &addrlen);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        filc_set_errno(my_errno);
        return result;
    }
    
    PAS_ASSERT(addrlen <= MAX_SOCKADDRLEN);

    filc_check_write_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
    filc_check_write_int(musl_addr_ptr, musl_addrlen, NULL);
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
    if (filc_check_and_clear(&musl_flags, 0x0001))
        *result |= MSG_OOB;
    if (filc_check_and_clear(&musl_flags, 0x0002))
        *result |= MSG_PEEK;
    if (filc_check_and_clear(&musl_flags, 0x0004))
        *result |= MSG_DONTROUTE;
    if (filc_check_and_clear(&musl_flags, 0x0008))
        *result |= MSG_CTRUNC;
    if (filc_check_and_clear(&musl_flags, 0x0020))
        *result |= MSG_TRUNC;
    if (filc_check_and_clear(&musl_flags, 0x0040))
        *result |= MSG_DONTWAIT;
    if (filc_check_and_clear(&musl_flags, 0x0080))
        *result |= MSG_EOR;
    if (filc_check_and_clear(&musl_flags, 0x0100))
        *result |= MSG_WAITALL;
    if (filc_check_and_clear(&musl_flags, 0x4000))
        *result |= MSG_NOSIGNAL;
    return !musl_flags;
}

static bool to_musl_msg_flags(int flags, int* result)
{
    *result = 0;
    if (filc_check_and_clear(&flags, MSG_OOB))
        *result |= 0x0001;
    if (filc_check_and_clear(&flags, MSG_PEEK))
        *result |= 0x0002;
    if (filc_check_and_clear(&flags, MSG_DONTROUTE))
        *result |= 0x0004;
    if (filc_check_and_clear(&flags, MSG_CTRUNC))
        *result |= 0x0008;
    if (filc_check_and_clear(&flags, MSG_TRUNC))
        *result |= 0x0020;
    if (filc_check_and_clear(&flags, MSG_DONTWAIT))
        *result |= 0x0040;
    if (filc_check_and_clear(&flags, MSG_EOR))
        *result |= 0x0080;
    if (filc_check_and_clear(&flags, MSG_WAITALL))
        *result |= 0x0100;
    if (filc_check_and_clear(&flags, MSG_NOSIGNAL))
        *result |= 0x4000;
    return !flags;
}

ssize_t filc_native_zsys_sendto(filc_thread* my_thread, int sockfd, filc_ptr buf_ptr, size_t len,
                                int musl_flags, filc_ptr musl_addr_ptr, unsigned musl_addrlen)
{
    filc_check_read_int(buf_ptr, len, NULL);
    filc_check_read_int(musl_addr_ptr, musl_addrlen, NULL);
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
        filc_set_errno(my_errno);

    bmalloc_deallocate(addr);
    return result;

einval:
    filc_set_errno(EINVAL);
    return -1;
}

ssize_t filc_native_zsys_recvfrom(filc_thread* my_thread, int sockfd, filc_ptr buf_ptr, size_t len,
                                  int musl_flags, filc_ptr musl_addr_ptr, filc_ptr musl_addrlen_ptr)
{
    filc_check_write_int(buf_ptr, len, NULL);
    unsigned musl_addrlen;
    if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_read_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
        musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
    } else
        musl_addrlen = 0;
    int flags;
    if (!from_musl_msg_flags(musl_flags, &flags)) {
        filc_set_errno(EINVAL);
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
        filc_set_errno(my_errno);
    else if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_write_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
        filc_check_write_int(musl_addr_ptr, musl_addrlen, NULL);
        struct musl_sockaddr* musl_addr = (struct musl_sockaddr*)filc_ptr_ptr(musl_addr_ptr);
        /* pass our own copy of musl_addrlen to avoid TOCTOU. */
        PAS_ASSERT(to_musl_sockaddr(my_thread, addr, addrlen, musl_addr, &musl_addrlen));
        *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr) = musl_addrlen;
    }

    return result;
}

static bool from_musl_ai_flags(int musl_flags, int* result)
{
    *result = 0;
    if (filc_check_and_clear(&musl_flags, 0x01))
        *result |= AI_PASSIVE;
    if (filc_check_and_clear(&musl_flags, 0x02))
        *result |= AI_CANONNAME;
    if (filc_check_and_clear(&musl_flags, 0x04))
        *result |= AI_NUMERICHOST;
#if !PAS_OS(OPENBSD)
    if (filc_check_and_clear(&musl_flags, 0x08))
        *result |= AI_V4MAPPED;
    if (filc_check_and_clear(&musl_flags, 0x10))
        *result |= AI_ALL;
#endif /* !PAS_OS(OPENBSD) */
    if (filc_check_and_clear(&musl_flags, 0x20))
        *result |= AI_ADDRCONFIG;
    if (filc_check_and_clear(&musl_flags, 0x400))
        *result |= AI_NUMERICSERV;
    return !musl_flags;
}

static bool to_musl_ai_flags(int flags, int* result)
{
    *result = 0;
    if (filc_check_and_clear(&flags, AI_PASSIVE))
        *result |= 0x01;
    if (filc_check_and_clear(&flags, AI_CANONNAME))
        *result |= 0x02;
    if (filc_check_and_clear(&flags, AI_NUMERICHOST))
        *result |= 0x04;
#if !PAS_OS(OPENBSD)
    if (filc_check_and_clear(&flags, AI_V4MAPPED))
        *result |= 0x08;
    if (filc_check_and_clear(&flags, AI_ALL))
        *result |= 0x10;
#endif /* !PAS_OS(OPENBSD) */
    if (filc_check_and_clear(&flags, AI_ADDRCONFIG))
        *result |= 0x20;
    if (filc_check_and_clear(&flags, AI_NUMERICSERV))
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
        filc_set_errno(errno);
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
        check_musl_addrinfo(hints_ptr, filc_read_access);
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
    char* node = filc_check_and_get_tmp_str_or_null(my_thread, node_ptr);
    char* service = filc_check_and_get_tmp_str_or_null(my_thread, service_ptr);
    struct addrinfo* res = NULL;
    int result;
    filc_exit(my_thread);
    result = getaddrinfo(node, service, &hints, &res);
    filc_enter(my_thread);
    if (result)
        return to_musl_eai(result);
    filc_check_write_ptr(res_ptr, NULL);
    filc_ptr* addrinfo_ptr = (filc_ptr*)filc_ptr_ptr(res_ptr);
    struct addrinfo* current;
    for (current = res; current; current = current->ai_next) {
        filc_ptr musl_current_ptr = filc_ptr_create(
            my_thread, filc_allocate(my_thread, sizeof(struct musl_addrinfo)));
        check_musl_addrinfo(musl_current_ptr, filc_write_access);
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
    return 0;
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
    filc_check_write_int(buf_ptr, sizeof(struct musl_utsname), NULL);
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

filc_ptr filc_native_zsys_getpwnam(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct passwd* passwd = getpwnam(name);
    int my_errno = errno;
    if (!passwd) {
        filc_set_errno(my_errno);
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
    filc_check_read_int(list_ptr, total_size, NULL);
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
        filc_set_errno(my_errno);
    return result;
}

filc_ptr filc_native_zsys_opendir(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    filc_exit(my_thread);
    DIR* dir = opendir(name);
    int my_errno = errno;
    filc_enter(my_thread);
    if (!dir) {
        filc_set_errno(my_errno);
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
        filc_set_errno(my_errno);
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
    bmalloc_initialize_allocation_config(&allocation_config);
    bmalloc_deallocate(dirstream->musl_to_loc);
    pas_uint64_hash_map_destruct(&dirstream->loc_to_musl, &allocation_config);
    pas_lock_unlock(&dirstream->lock);
    filc_free_yolo(my_thread, filc_object_for_special_payload(dirstream));

    filc_exit(my_thread);
    int result = closedir(dir);
    int my_errno = errno;
    filc_enter(my_thread);

    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

static uint64_t to_musl_dirstream_loc(zdirstream* stream, uint64_t loc)
{
    pas_uint64_hash_map_add_result add_result;
    pas_allocation_config allocation_config;

    PAS_ASSERT(stream->loc_to_musl.table_size == stream->musl_to_loc_size);
        
    bmalloc_initialize_allocation_config(&allocation_config);

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
        filc_set_errno(my_errno);
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
        filc_set_errno(my_errno);
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
    char* ident = filc_check_and_get_tmp_str(my_thread, ident_ptr);
    filc_exit(my_thread);
    /* Amazingly, the option/facility constants all match up! */
    openlog(ident, option, facility);
    filc_enter(my_thread);
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
    char* msg = filc_check_and_get_tmp_str(my_thread, msg_ptr);
    filc_exit(my_thread);
    syslog(priority, "%s", msg);
    filc_enter(my_thread);
}

int filc_native_zsys_accept(filc_thread* my_thread, int sockfd, filc_ptr musl_addr_ptr,
                            filc_ptr musl_addrlen_ptr)
{
    unsigned musl_addrlen;
    if (filc_ptr_ptr(musl_addrlen_ptr)) {
        filc_check_read_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
        musl_addrlen = *(unsigned*)filc_ptr_ptr(musl_addrlen_ptr);
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
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(musl_addrlen_ptr, sizeof(unsigned), NULL);
        filc_check_write_int(musl_addr_ptr, musl_addrlen, NULL);
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
    filc_check_write_int(sv_ptr, sizeof(int) * 2, NULL);
    int domain;
    if (!from_musl_domain(musl_domain, &domain)) {
        filc_set_errno(EINVAL);
        return -1;
    }
    int type;
    if (!from_musl_socket_type(musl_type, &type)) {
        filc_set_errno(EINVAL);
        return -1;
    }
    filc_pin(filc_ptr_object(sv_ptr));
    filc_exit(my_thread);
    int result = socketpair(domain, type, protocol, (int*)filc_ptr_ptr(sv_ptr));
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(sv_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_getgrouplist(filc_thread* my_thread, filc_ptr user_ptr, unsigned group,
                                  filc_ptr groups_ptr, filc_ptr ngroups_ptr)
{
    static const bool verbose = false;
    
    char* user = filc_check_and_get_tmp_str(my_thread, user_ptr);
    filc_check_read_int(ngroups_ptr, sizeof(int), NULL);
    int ngroups = *(int*)filc_ptr_ptr(ngroups_ptr);
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(sizeof(unsigned), ngroups, &total_size),
        NULL,
        "ngroups argument too big, causes overflow; ngroups = %d.",
        ngroups);
    filc_check_write_int(groups_ptr, total_size, NULL);
    filc_pin(filc_ptr_object(groups_ptr));
    filc_exit(my_thread);
    if (verbose)
        pas_log("ngroups = %d\n", ngroups);
    int result = getgrouplist(user, group, filc_ptr_ptr(groups_ptr), &ngroups);
    if (verbose)
        pas_log("ngroups after = %d\n", ngroups);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(groups_ptr));
    if (verbose)
        pas_log("result = %d, error = %s\n", result, strerror(my_errno));
    if (result < 0)
        filc_set_errno(my_errno);
    filc_check_write_int(ngroups_ptr, sizeof(int), NULL);
    *(int*)filc_ptr_ptr(ngroups_ptr) = ngroups;
    return result;
}

int filc_native_zsys_initgroups(filc_thread* my_thread, filc_ptr user_ptr, unsigned gid)
{
    static const bool verbose = false;
    
    char* user = filc_check_and_get_tmp_str(my_thread, user_ptr);
    filc_exit(my_thread);
    int result = initgroups(user, gid);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_openpty(filc_thread* my_thread, filc_ptr masterfd_ptr, filc_ptr slavefd_ptr,
                             filc_ptr name_ptr, filc_ptr musl_term_ptr, filc_ptr musl_win_ptr)
{
    filc_check_write_int(masterfd_ptr, sizeof(int), NULL);
    filc_check_write_int(slavefd_ptr, sizeof(int), NULL);
    FILC_ASSERT(!filc_ptr_ptr(name_ptr), NULL);
    struct termios* term = NULL;
    if (filc_ptr_ptr(musl_term_ptr)) {
        filc_check_read_int(musl_term_ptr, sizeof(struct musl_termios), NULL);
        term = alloca(sizeof(struct termios));
        from_musl_termios((struct musl_termios*)filc_ptr_ptr(musl_term_ptr), term);
    }
    struct winsize* win = NULL;
    if (filc_ptr_ptr(musl_win_ptr)) {
        filc_check_read_int(musl_win_ptr, sizeof(struct musl_winsize), NULL);
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
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_ttyname_r(filc_thread* my_thread, int fd, filc_ptr buf_ptr, size_t buflen)
{
    filc_check_write_int(buf_ptr, buflen, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    int result = ttyname_r(fd, (char*)filc_ptr_ptr(buf_ptr), buflen);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

static struct filc_ptr to_musl_group(filc_thread* my_thread, struct group* group)
{
    filc_ptr result_ptr = filc_ptr_create(my_thread, filc_allocate(my_thread, sizeof(struct musl_group)));
    check_musl_group(result_ptr, filc_write_access);
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
            filc_check_write_ptr(filc_ptr_with_ptr(musl_mem_ptr, musl_mem + index), NULL);
            filc_ptr_store(my_thread, musl_mem + index, filc_strdup(my_thread, group->gr_mem[index]));
        }
        filc_ptr_store(my_thread, &result->gr_mem, musl_mem_ptr);
    }
    
    return result_ptr;
}

filc_ptr filc_native_zsys_getgrnam(filc_thread* my_thread, filc_ptr name_ptr)
{
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    /* Don't filc_exit so we don't have a reentrancy problem on the thread-local passwd. */
    struct group* group = getgrnam(name);
    int my_errno = errno;
    if (!group) {
        filc_set_errno(my_errno);
        return filc_ptr_forge_null();
    }
    /* FIXME: a signal that calls getgrnam here could cause a reentrancy issue, maybe.
     
       We could address that with a reentrancy "lock" per thread that is just a bit saying, "we're doing
       stuff to grnam on this thread so if you try to do it then get fucked". */
    return to_musl_group(my_thread, group);
}

#if HAVE_UTMPX
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
    filc_user_timeval ut_tv;
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
    memcpy(utmpx->ut_id, musl_utmpx->ut_id, pas_min_uintptr(sizeof(utmpx->ut_id),
                                                            sizeof(musl_utmpx->ut_id)));
    utmpx->ut_tv.tv_sec = musl_utmpx->ut_tv.tv_sec;
    utmpx->ut_tv.tv_usec = musl_utmpx->ut_tv.tv_usec;
    utmpx->ut_pid = musl_utmpx->ut_pid;
    switch (musl_utmpx->ut_type) {
    case 0:
        utmpx->ut_type = EMPTY;
        break;
#if PAS_OS(DARWIN)
    case 1:
        utmpx->ut_type = RUN_LVL;
        break;
#endif /* PAS_OS(DARWIN) */
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
    memcpy(musl_utmpx->ut_id, utmpx->ut_id, pas_min_uintptr(sizeof(utmpx->ut_id),
                                                            sizeof(musl_utmpx->ut_id)));
    musl_utmpx->ut_tv.tv_sec = utmpx->ut_tv.tv_sec;
    musl_utmpx->ut_tv.tv_usec = utmpx->ut_tv.tv_usec;
    musl_utmpx->ut_pid = utmpx->ut_pid;
    switch (utmpx->ut_type) {
    case EMPTY:
        musl_utmpx->ut_type = 0;
        break;
#if PAS_OS(DARWIN)
    case RUN_LVL:
        musl_utmpx->ut_type = 1;
        break;
#endif /* PAS_OS(DARWIN) */
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
        filc_set_errno(errno);
        return filc_ptr_forge_null();
    }
    filc_ptr result = filc_ptr_create(my_thread, filc_allocate_int(my_thread, sizeof(struct musl_utmpx)));
    to_musl_utmpx(utmpx, (struct musl_utmpx*)filc_ptr_ptr(result));
    return result;
}

#endif /* HAVE_UTMPX */

void filc_native_zsys_endutxent(filc_thread* my_thread)
{
#if HAVE_UTMPX
    PAS_UNUSED_PARAM(my_thread);
    pas_lock_lock(&utmpx_lock);
    endutxent();
    pas_lock_unlock(&utmpx_lock);
#else /* HAVE_UTMPX -> so !HAVE_UTMPX */
    PAS_UNUSED_PARAM(my_thread);
    filc_internal_panic(NULL, "not implemented.");
#endif /* HAVE_UTMPX -> so end of !HAVE_UTMPX */
}

filc_ptr filc_native_zsys_getutxent(filc_thread* my_thread)
{
#if HAVE_UTMPX
    pas_lock_lock(&utmpx_lock);
    filc_ptr result = handle_utmpx_result(my_thread, getutxent());
    pas_lock_unlock(&utmpx_lock);
    return result;
#else /* HAVE_UTMPX -> so !HAVE_UTMPX */
    PAS_UNUSED_PARAM(my_thread);
    filc_internal_panic(NULL, "not implemented.");
    return filc_ptr_forge_null();
#endif /* HAVE_UTMPX -> so end of !HAVE_UTMPX */
}

filc_ptr filc_native_zsys_getutxid(filc_thread* my_thread, filc_ptr musl_utmpx_ptr)
{
#if HAVE_UTMPX
    filc_check_write_int(musl_utmpx_ptr, sizeof(struct musl_utmpx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct utmpx utmpx_in;
    from_musl_utmpx((struct musl_utmpx*)filc_ptr_ptr(musl_utmpx_ptr), &utmpx_in);
    filc_ptr result = handle_utmpx_result(my_thread, getutxid(&utmpx_in));
    pas_lock_unlock(&utmpx_lock);
    return result;
#else /* HAVE_UTMPX -> so !HAVE_UTMPX */
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(musl_utmpx_ptr);
    filc_internal_panic(NULL, "not implemented.");
    return filc_ptr_forge_null();
#endif /* HAVE_UTMPX -> so end of !HAVE_UTMPX */
}

filc_ptr filc_native_zsys_getutxline(filc_thread* my_thread, filc_ptr musl_utmpx_ptr)
{
#if HAVE_UTMPX
    filc_check_write_int(musl_utmpx_ptr, sizeof(struct musl_utmpx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct utmpx utmpx_in;
    from_musl_utmpx((struct musl_utmpx*)filc_ptr_ptr(musl_utmpx_ptr), &utmpx_in);
    filc_ptr result = handle_utmpx_result(my_thread, getutxline(&utmpx_in));
    pas_lock_unlock(&utmpx_lock);
    return result;
#else /* HAVE_UTMPX -> so !HAVE_UTMPX */
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(musl_utmpx_ptr);
    filc_internal_panic(NULL, "not implemented.");
    return filc_ptr_forge_null();
#endif /* HAVE_UTMPX -> so end of !HAVE_UTMPX */
}

filc_ptr filc_native_zsys_pututxline(filc_thread* my_thread, filc_ptr musl_utmpx_ptr)
{
#if HAVE_UTMPX
    filc_check_write_int(musl_utmpx_ptr, sizeof(struct musl_utmpx), NULL);
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
#else /* HAVE_UTMPX -> so !HAVE_UTMPX */
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(musl_utmpx_ptr);
    filc_internal_panic(NULL, "not implemented.");
    return filc_ptr_forge_null();
#endif /* HAVE_UTMPX -> so end of !HAVE_UTMPX */
}

void filc_native_zsys_setutxent(filc_thread* my_thread)
{
#if HAVE_UTMPX
    PAS_UNUSED_PARAM(my_thread);
    pas_lock_lock(&utmpx_lock);
    setutxent();
    pas_lock_unlock(&utmpx_lock);
#else /* HAVE_UTMPX -> so !HAVE_UTMPX */
    PAS_UNUSED_PARAM(my_thread);
    filc_internal_panic(NULL, "not implemented.");
#endif /* HAVE_UTMPX -> so end of !HAVE_UTMPX */
}

#if HAVE_LASTLOGX
struct musl_lastlogx {
    filc_user_timeval ll_tv;
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
        filc_set_errno(errno);
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
#endif /* HAVE_LASTLOGX */

filc_ptr filc_native_zsys_getlastlogx(filc_thread* my_thread, unsigned uid, filc_ptr musl_lastlogx_ptr)
{
#if HAVE_LASTLOGX
    if (filc_ptr_ptr(musl_lastlogx_ptr))
        filc_check_write_int(musl_lastlogx_ptr, sizeof(struct musl_lastlogx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct lastlogx lastlogx;
    filc_ptr result = handle_lastlogx_result(my_thread, musl_lastlogx_ptr, getlastlogx(uid, &lastlogx));
    pas_lock_unlock(&utmpx_lock);
    return result;
#else /* HAVE_LASTLOGX -> so !HAVE_LASTLOGX */
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(uid);
    PAS_UNUSED_PARAM(musl_lastlogx_ptr);
    filc_internal_panic(NULL, "not implemented.");
    return filc_ptr_forge_null();
#endif /* HAVE_LASTLOGX -> so end of !HAVE_LASTLOGX */
}

filc_ptr filc_native_zsys_getlastlogxbyname(filc_thread* my_thread, filc_ptr name_ptr,
                                            filc_ptr musl_lastlogx_ptr)
{
#if HAVE_LASTLOGX
    char* name = filc_check_and_get_tmp_str(my_thread, name_ptr);
    if (filc_ptr_ptr(musl_lastlogx_ptr))
        filc_check_write_int(musl_lastlogx_ptr, sizeof(struct musl_lastlogx), NULL);
    pas_lock_lock(&utmpx_lock);
    struct lastlogx lastlogx;
    filc_ptr result = handle_lastlogx_result(
        my_thread, musl_lastlogx_ptr, getlastlogxbyname(name, &lastlogx));
    pas_lock_unlock(&utmpx_lock);
    return result;
#else /* HAVE_LASTLOGX -> so !HAVE_LASTLOGX */
    PAS_UNUSED_PARAM(my_thread);
    PAS_UNUSED_PARAM(name_ptr);
    PAS_UNUSED_PARAM(musl_lastlogx_ptr);
    filc_internal_panic(NULL, "not implemented.");
    return filc_ptr_forge_null();
#endif /* HAVE_LASTLOGX -> so end of !HAVE_LASTLOGX */
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

static void check_musl_msghdr(filc_ptr ptr, filc_access_kind access_kind)
{
    FILC_CHECK_PTR_FIELD(ptr, struct musl_msghdr, msg_name, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_namelen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_msghdr, msg_iov, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_iovlen, access_kind);
    FILC_CHECK_PTR_FIELD(ptr, struct musl_msghdr, msg_control, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_controllen, access_kind);
    FILC_CHECK_INT_FIELD(ptr, struct musl_msghdr, msg_flags, access_kind);
}

struct musl_cmsghdr {
    unsigned cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

static void destroy_msghdr(struct msghdr* msghdr)
{
    bmalloc_deallocate(msghdr->msg_name);
    bmalloc_deallocate(msghdr->msg_control);
}

static void from_musl_msghdr_base(filc_thread* my_thread,
                                  struct musl_msghdr* musl_msghdr, struct msghdr* msghdr,
                                  filc_access_kind access_kind)
{
    pas_zero_memory(msghdr, sizeof(struct msghdr));

    int iovlen = musl_msghdr->msg_iovlen;
    msghdr->msg_iov = filc_prepare_iovec(
        my_thread, filc_ptr_load(my_thread, &musl_msghdr->msg_iov), iovlen, access_kind);
    msghdr->msg_iovlen = iovlen;

    msghdr->msg_flags = 0; /* This field is ignored so just zero-init it. */
}

static bool from_musl_msghdr_for_send(filc_thread* my_thread,
                                      struct musl_msghdr* musl_msghdr, struct msghdr* msghdr)
{
    static const bool verbose = false;

    if (verbose)
        pas_log("In from_musl_msghdr_for_send\n");
    
    from_musl_msghdr_base(my_thread, musl_msghdr, msghdr, filc_read_access);

    unsigned msg_namelen = musl_msghdr->msg_namelen;
    if (msg_namelen) {
        filc_ptr msg_name = filc_ptr_load(my_thread, &musl_msghdr->msg_name);
        filc_check_read_int(msg_name, msg_namelen, NULL);
        if (!from_musl_sockaddr((struct musl_sockaddr*)filc_ptr_ptr(msg_name), msg_namelen,
                                (struct sockaddr**)&msghdr->msg_name, &msghdr->msg_namelen))
            goto error;
    }

    unsigned musl_controllen = musl_msghdr->msg_controllen;
    if (musl_controllen) {
        filc_ptr musl_control = filc_ptr_load(my_thread, &musl_msghdr->msg_control);
        filc_check_read_int(musl_control, musl_controllen, NULL);
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
    destroy_msghdr(msghdr);
    return false;
}

static bool from_musl_msghdr_for_recv(filc_thread* my_thread,
                                      struct musl_msghdr* musl_msghdr, struct msghdr* msghdr)
{
    from_musl_msghdr_base(my_thread, musl_msghdr, msghdr, filc_write_access);

    unsigned musl_namelen = musl_msghdr->msg_namelen;
    if (musl_namelen) {
        filc_check_read_int(filc_ptr_load(my_thread, &musl_msghdr->msg_name), musl_namelen, NULL);
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
        filc_check_write_int(musl_name_ptr, musl_namelen, NULL);
        struct musl_sockaddr* musl_name = (struct musl_sockaddr*)filc_ptr_ptr(musl_name_ptr);
        PAS_ASSERT(to_musl_sockaddr(my_thread, msghdr->msg_name, msghdr->msg_namelen,
                                    musl_name, &musl_namelen));
        musl_msghdr->msg_namelen = musl_namelen;
    }
    
    unsigned musl_controllen = musl_msghdr->msg_controllen;
    filc_ptr musl_control = filc_ptr_load(my_thread, &musl_msghdr->msg_control);
    filc_check_write_int(musl_control, musl_controllen, NULL);
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

    check_musl_msghdr(msg_ptr, filc_read_access);
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
    destroy_msghdr(&msg);
    if (result < 0)
        filc_set_errno(my_errno);
    return result;

einval:
    filc_set_errno(EINVAL);
    return -1;
}

ssize_t filc_native_zsys_recvmsg(filc_thread* my_thread, int sockfd, filc_ptr msg_ptr, int musl_flags)
{
    static const bool verbose = false;

    check_musl_msghdr(msg_ptr, filc_read_access);
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
        pas_log("msg.msg_iovlen = %zu\n", (size_t)msg.msg_iovlen);
        int index;
        for (index = 0; index < (int)msg.msg_iovlen; ++index)
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
        filc_set_errno(my_errno);
    } else {
        check_musl_msghdr(msg_ptr, filc_write_access);
        to_musl_msghdr_for_recv(my_thread, &msg, musl_msg);
    }
    destroy_msghdr(&msg);
    return result;

einval:
    filc_set_errno(EINVAL);
    return -1;
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
#if PAS_OS(DARWIN)
    case 87:
        name = _SC_PASS_MAX;
        break;
#endif /* PAS_OS(DARWIN) */
    case 89:
        name = _SC_XOPEN_VERSION;
        break;
#if !PAS_OS(OPENBSD)
    case 90:
        name = _SC_XOPEN_XCU_VERSION;
        break;
#endif /* !PAS_OS(OPENBSD) */
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
        filc_set_errno(ENOSYS);
        return -1;
    }
    filc_exit(my_thread);
    errno = 0;
    long result = sysconf(name);
    int my_errno = errno;
    filc_enter(my_thread);
    if (my_errno)
        filc_set_errno(my_errno);
    return result;
}

int filc_native_zsys_numcores(filc_thread* my_thread)
{
#if PAS_OS(LINUX)
    filc_exit(my_thread);
    int result = sysconf(_SC_NPROCESSORS_ONLN);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0) {
        filc_set_errno(my_errno);
        return -1;
    }
    return result;
#else /* PAS_OS(LINUX) -> so !PAS_OS(LINUX) */
    filc_exit(my_thread);
    unsigned result;
    size_t length = sizeof(result);
    int name[] = {
        CTL_HW,
#if PAS_OS(DARWIN)
        HW_AVAILCPU
#else /* PAS_OS(DARWIN) -> so !PAS_OS(DARWIN) */
        HW_NCPU
#endif /* PAS_OS(DARWIN) -> so end of !PAS_OS(DARWIN) */
    };
    int sysctl_result = sysctl(name, sizeof(name) / sizeof(int), &result, &length, 0, 0);
    int my_errno = errno;
    filc_enter(my_thread);
    if (sysctl_result < 0) {
        filc_set_errno(my_errno);
        return -1;
    }
    PAS_ASSERT((int)result >= 0);
    return (int)result;
#endif /* PAS_OS(LINUX) -> so end of !PAS_OS(LINUX) */
}

int filc_native_zsys_getentropy(filc_thread* my_thread, filc_ptr buf_ptr, size_t len)
{
    filc_check_write_int(buf_ptr, len, NULL);
    filc_pin(filc_ptr_object(buf_ptr));
    filc_exit(my_thread);
    int result = getentropy(filc_ptr_ptr(buf_ptr), len);
    int my_errno = errno;
    filc_enter(my_thread);
    filc_unpin(filc_ptr_object(buf_ptr));
    if (result < 0)
        filc_set_errno(my_errno);
    return result;
}

struct musl_posix_spawnattr {
    int flags;
    int pgrp;
    filc_user_sigset def;
    filc_user_sigset mask;
    int prio;
    int pol;
};

struct musl_posix_spawn_file_actions {
    int pad0[2];
    filc_ptr actions;
};

struct musl_fdop {
    filc_ptr next;
    filc_ptr prev;
    int cmd;
    int fd;
    int srcfd;
    int oflag;
    unsigned mode;
    char path[];
};

static bool from_musl_posix_spawnattr(filc_ptr musl_spawnattr_ptr,
                                      posix_spawnattr_t* spawnattr)
{
    PAS_ASSERT(!posix_spawnattr_init(spawnattr));

    if (!filc_ptr_ptr(musl_spawnattr_ptr))
        return true;

    filc_check_read_int(musl_spawnattr_ptr, sizeof(struct musl_posix_spawnattr), NULL);
    struct musl_posix_spawnattr* musl_spawnattr =
        (struct musl_posix_spawnattr*)filc_ptr_ptr(musl_spawnattr_ptr);
    
    unsigned musl_flags = musl_spawnattr->flags;
    unsigned flags = 0;
    if (filc_check_and_clear(&musl_flags, 1))
        flags |= POSIX_SPAWN_RESETIDS;
    if (filc_check_and_clear(&musl_flags, 2))
        flags |= POSIX_SPAWN_SETPGROUP;
    if (filc_check_and_clear(&musl_flags, 4))
        flags |= POSIX_SPAWN_SETSIGDEF;
    if (filc_check_and_clear(&musl_flags, 8))
        flags |= POSIX_SPAWN_SETSIGMASK;
#if PAS_OS(DARWIN)
    if (filc_check_and_clear(&musl_flags, 128))
        flags |= POSIX_SPAWN_SETSID;
#endif
    if (musl_flags)
        return false;
    PAS_ASSERT(!posix_spawnattr_setflags(spawnattr, flags));

    sigset_t sigset;
    filc_from_user_sigset(&musl_spawnattr->def, &sigset);
    PAS_ASSERT(!posix_spawnattr_setsigdefault(spawnattr, &sigset));

    filc_from_user_sigset(&musl_spawnattr->mask, &sigset);
    PAS_ASSERT(!posix_spawnattr_setsigmask(spawnattr, &sigset));

    PAS_ASSERT(!posix_spawnattr_setpgroup(spawnattr, musl_spawnattr->pgrp));
    return true;
}

static bool from_musl_posix_spawn_file_actions(filc_thread* my_thread,
                                               filc_ptr musl_actions_ptr,
                                               posix_spawn_file_actions_t* actions)
{
    PAS_ASSERT(!posix_spawn_file_actions_init(actions));

    if (!filc_ptr_ptr(musl_actions_ptr))
        return true;

    FILC_CHECK_PTR_FIELD(musl_actions_ptr, struct musl_posix_spawn_file_actions, actions,
                         filc_read_access);
    struct musl_posix_spawn_file_actions* musl_actions =
        (struct musl_posix_spawn_file_actions*)filc_ptr_ptr(musl_actions_ptr);
    filc_ptr fdop_ptr = filc_ptr_load(my_thread, &musl_actions->actions);
    if (!filc_ptr_ptr(fdop_ptr))
        return true;
    
    for (;;) {
        FILC_CHECK_PTR_FIELD(fdop_ptr, struct musl_fdop, next, filc_read_access);
        struct musl_fdop* fdop = (struct musl_fdop*)filc_ptr_ptr(fdop_ptr);
        filc_ptr next_ptr = filc_ptr_load(my_thread, &fdop->next);
        if (!filc_ptr_ptr(next_ptr))
            break;
        fdop_ptr = next_ptr;
    }
    do {
        FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, cmd, filc_read_access);
        struct musl_fdop* fdop = (struct musl_fdop*)filc_ptr_ptr(fdop_ptr);
        switch (fdop->cmd) {
        case 1: /* FDOP_CLOSE */ {
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, fd, filc_read_access);
            PAS_ASSERT(!posix_spawn_file_actions_addclose(actions, fdop->fd));
            break;
        }
        case 2: /* FDOP_DUP2 */ {
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, srcfd, filc_read_access);
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, fd, filc_read_access);
            PAS_ASSERT(!posix_spawn_file_actions_adddup2(actions, fdop->srcfd, fdop->fd));
            break;
        }
        case 3: /* FDOP_OPEN */ {
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, fd, filc_read_access);
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, oflag, filc_read_access);
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, mode, filc_read_access);
            int flags = filc_from_user_open_flags(fdop->oflag);
            if (flags < 0)
                goto error;
            char* path = filc_check_and_get_tmp_str(
                my_thread, filc_ptr_with_offset(fdop_ptr, PAS_OFFSETOF(struct musl_fdop, path)));
            PAS_ASSERT(!posix_spawn_file_actions_addopen(actions, fdop->fd, path, flags, fdop->mode));
            break;
        }
#if !PAS_OS(OPENBSD)
        case 4: /* FDOP_CHDIR */ {
            char* path = filc_check_and_get_tmp_str(
                my_thread, filc_ptr_with_offset(fdop_ptr, PAS_OFFSETOF(struct musl_fdop, path)));
            PAS_ASSERT(!posix_spawn_file_actions_addchdir_np(actions, path));
            break;
        }
        case 5: /* FDOP_FCHDIR */ {
            FILC_CHECK_INT_FIELD(fdop_ptr, struct musl_fdop, fd, filc_read_access);
            PAS_ASSERT(!posix_spawn_file_actions_addfchdir_np(actions, fdop->fd));
            break;
        }
#endif /* !PAS_OS(OPENBSD) */
        default:
            goto error;
        }
        FILC_CHECK_PTR_FIELD(fdop_ptr, struct musl_fdop, prev, filc_read_access);
        fdop_ptr = filc_ptr_load(my_thread, &fdop->prev);
    } while (filc_ptr_ptr(fdop_ptr));

    return true;

error:
    posix_spawn_file_actions_destroy(actions);
    return false;
}

static int posix_spawn_impl(filc_thread* my_thread, filc_ptr pid_ptr, filc_ptr path_ptr,
                            filc_ptr actions_ptr, filc_ptr attr_ptr, filc_ptr argv_ptr,
                            filc_ptr envp_ptr, bool is_p)
{
    char* path = filc_check_and_get_tmp_str(my_thread, path_ptr);
    posix_spawn_file_actions_t actions;
    if (!from_musl_posix_spawn_file_actions(my_thread, actions_ptr, &actions)) {
        filc_set_errno(EINVAL);
        return -1;
    }
    posix_spawnattr_t spawnattr;
    if (!from_musl_posix_spawnattr(attr_ptr, &spawnattr)) {
        posix_spawn_file_actions_destroy(&actions);
        filc_set_errno(EINVAL);
        return -1;
    }
    char** argv = filc_check_and_get_null_terminated_string_array(my_thread, argv_ptr);
    char** envp = filc_check_and_get_null_terminated_string_array(my_thread, envp_ptr);
    int pid;
    filc_exit(my_thread);
    int result;
    if (is_p)
        result = posix_spawnp(&pid, path, &actions, &spawnattr, argv, envp);
    else
        result = posix_spawn(&pid, path, &actions, &spawnattr, argv, envp);
    filc_enter(my_thread);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&spawnattr);
    if (result)
        return filc_to_user_errno(result);
    filc_check_write_int(pid_ptr, sizeof(int), NULL);
    *(int*)filc_ptr_ptr(pid_ptr) = pid;
    return 0;
}

int filc_native_zsys_posix_spawn(filc_thread* my_thread, filc_ptr pid_ptr, filc_ptr path_ptr,
                                 filc_ptr actions_ptr, filc_ptr attr_ptr, filc_ptr argv_ptr,
                                 filc_ptr envp_ptr)
{
    bool is_p = false;
    return posix_spawn_impl(
        my_thread, pid_ptr, path_ptr, actions_ptr, attr_ptr, argv_ptr, envp_ptr, is_p);
}

int filc_native_zsys_posix_spawnp(filc_thread* my_thread, filc_ptr pid_ptr, filc_ptr path_ptr,
                                  filc_ptr actions_ptr, filc_ptr attr_ptr, filc_ptr argv_ptr,
                                  filc_ptr envp_ptr)
{
    bool is_p = true;
    return posix_spawn_impl(
        my_thread, pid_ptr, path_ptr, actions_ptr, attr_ptr, argv_ptr, envp_ptr, is_p);
}

struct musl_flock {
    short l_type;
    short l_whence;
    int64_t l_start;
    int64_t l_len;
    int l_pid;
};

int filc_native_zsys_fcntl(filc_thread* my_thread, int fd, int musl_cmd, filc_cc_cursor* args)
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
    struct flock saved_flock;
    filc_ptr musl_flock_ptr;
    struct musl_flock* musl_flock;
    pas_zero_memory(&arg_flock, sizeof(arg_flock));
    if (have_arg_int)
        arg_int = filc_cc_cursor_get_next_int(args, int);
    if (have_arg_flock) {
        musl_flock_ptr = filc_cc_cursor_get_next_ptr(args);
        filc_check_read_int(musl_flock_ptr, sizeof(struct musl_flock), NULL);
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
            filc_set_errno(EINVAL);
            return -1;
        }
        arg_flock.l_whence = musl_flock->l_whence;
        arg_flock.l_start = musl_flock->l_start;
        arg_flock.l_len = musl_flock->l_len;
        arg_flock.l_pid = musl_flock->l_pid;
        saved_flock = arg_flock;
    }
    if (!have_cmd) {
        if (verbose)
            pas_log("no cmd!\n");
        filc_set_errno(EINVAL);
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
            filc_set_errno(EINVAL);
            return -1;
        }
        break;
    case F_SETFL:
        arg_int = filc_from_user_open_flags(arg_int);
        break;
    default:
        break;
    }
    int result;
    filc_exit(my_thread);
    if (have_arg_int) {
        PAS_ASSERT(!have_arg_flock);
        result = fcntl(fd, cmd, arg_int);
    } else if (have_arg_flock)
        result = fcntl(fd, cmd, &arg_flock);
    else
        result = fcntl(fd, cmd);
    int my_errno = errno;
    filc_enter(my_thread);
    if (verbose)
        pas_log("result = %d\n", result);
    if (have_arg_flock) {
        filc_check_write_int(musl_flock_ptr, sizeof(struct musl_flock), NULL);
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
    }
    if (result == -1) {
        if (verbose)
            pas_log("error = %s\n", strerror(my_errno));
        filc_set_errno(my_errno);
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
        result = filc_to_user_open_flags(result);
        break;
    default:
        break;
    }
    return result;
}

static bool from_musl_poll_events(short musl_events, short* events)
{
    *events = 0;
    if (filc_check_and_clear(&musl_events, 0x001))
        *events |= POLLIN;
    if (filc_check_and_clear(&musl_events, 0x002))
        *events |= POLLPRI;
    if (filc_check_and_clear(&musl_events, 0x004))
        *events |= POLLOUT;
    if (filc_check_and_clear(&musl_events, 0x008))
        *events |= POLLERR;
    if (filc_check_and_clear(&musl_events, 0x010))
        *events |= POLLHUP;
    if (filc_check_and_clear(&musl_events, 0x020))
        *events |= POLLNVAL;
    if (filc_check_and_clear(&musl_events, 0x040))
        *events |= POLLRDNORM;
    if (filc_check_and_clear(&musl_events, 0x080))
        *events |= POLLRDBAND;
    if (filc_check_and_clear(&musl_events, 0x100))
        *events |= POLLWRNORM;
    if (filc_check_and_clear(&musl_events, 0x200))
        *events |= POLLWRBAND;
    return !musl_events;
}

static void to_musl_poll_revents(short musl_events, short revents, short* musl_revents)
{
    *musl_revents = 0;
    if (filc_check_and_clear(&revents, POLLIN)) {
        if (!(musl_events & 0x001) && (musl_events & 0x040))
            *musl_revents |= 0x040;
        else
            *musl_revents |= 0x001;
    }
    if (filc_check_and_clear(&revents, POLLPRI))
        *musl_revents |= 0x002;
    if (filc_check_and_clear(&revents, POLLOUT)) {
        if (!(musl_events & 0x004) && (musl_events & 0x100))
            *musl_revents |= 0x100;
        else
            *musl_revents |= 0x004;
    }
    if (filc_check_and_clear(&revents, POLLERR))
        *musl_revents |= 0x008;
    if (filc_check_and_clear(&revents, POLLHUP))
        *musl_revents |= 0x010;
    if (filc_check_and_clear(&revents, POLLNVAL))
        *musl_revents |= 0x020;
    if (filc_check_and_clear(&revents, POLLRDNORM))
        *musl_revents |= 0x040;
    if (filc_check_and_clear(&revents, POLLRDBAND))
        *musl_revents |= 0x080;
    if (filc_check_and_clear(&revents, POLLWRNORM))
        *musl_revents |= 0x100;
    if (filc_check_and_clear(&revents, POLLWRBAND))
        *musl_revents |= 0x200;
    PAS_ASSERT(!revents);
}

struct musl_pollfd {
    int fd;
    short events;
    short revents;
};

static struct pollfd* from_musl_pollfds(struct musl_pollfd* musl_pollfds, unsigned long nfds)
{
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(nfds, sizeof(struct pollfd), &total_size),
        NULL,
        "nfds so big that native pollfds size calculation overflowed.");
    struct pollfd* result = bmalloc_allocate(total_size);

    size_t index;
    for (index = nfds; index--;) {
        result[index].fd = musl_pollfds[index].fd;
        if (!from_musl_poll_events(musl_pollfds[index].events, &result[index].events))
            goto error;
        result[index].revents = 0;
    }
    
    return result;
    
error:
    bmalloc_deallocate(result);
    return NULL;
}

static void to_musl_pollfds(struct pollfd* pollfds, struct musl_pollfd* musl_pollfds, unsigned long nfds)
{
    size_t index;
    for (index = nfds; index--;) {
        to_musl_poll_revents(
            musl_pollfds[index].events, pollfds[index].revents, &musl_pollfds[index].revents);
    }
}

int filc_native_zsys_poll(
    filc_thread* my_thread, filc_ptr musl_pollfds_ptr, unsigned long nfds, int timeout)
{
    if (!nfds) {
        filc_exit(my_thread);
        int result = poll(NULL, 0, timeout);
        int my_errno = errno;
        filc_enter(my_thread);
        if (result < 0)
            filc_set_errno(my_errno);
        return result;
    }
    size_t total_size;
    FILC_CHECK(
        !pas_mul_uintptr_overflow(nfds, sizeof(struct musl_pollfd), &total_size),
        NULL,
        "nfds so big that pollfds size calculation overflowed.");
    filc_check_read_int(musl_pollfds_ptr, total_size, NULL);
    struct musl_pollfd* musl_pollfds = filc_ptr_ptr(musl_pollfds_ptr);
    struct pollfd* pollfds = from_musl_pollfds(musl_pollfds, nfds);
    if (!pollfds) {
        filc_set_errno(EINVAL);
        return -1;
    }
    filc_exit(my_thread);
    int result = poll(pollfds, nfds, timeout);
    int my_errno = errno;
    filc_enter(my_thread);
    if (result < 0)
        filc_set_errno(my_errno);
    else {
        filc_check_write_int(musl_pollfds_ptr, total_size, NULL);
        to_musl_pollfds(pollfds, musl_pollfds, nfds);
    }
    bmalloc_deallocate(pollfds);
    return result;
}

#endif /* PAS_ENABLE_FILC && FILC_NUSL */

#endif /* LIBPAS_ENABLED */

