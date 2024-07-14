#!/usr/bin/env ruby
#
# Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

def checkType(type)
    case type
    when 'filc_ptr', 'int', 'unsigned', 'long', 'unsigned long', 'size_t', 'double', 'bool',
         "ssize_t", 'unsigned short', 'unsigned long long', 'long long', 'pizlonated_mode_t'
    else
        raise "Bad type #{type}"
    end
end

def underbarType(type)
    if type == "filc_ptr"
        return "ptr"
    else
        type.gsub(/ /, '_')
    end
end

class Signature
    attr_reader :name, :args, :rets, :defines

    def initialize(name, args, rets, defines)
        @name = name
        @args = args
        @rets = rets
        @defines = defines

        args.each_with_index {
            | arg, index |
            if arg == "..."
                raise unless index == args.length - 1
            else
                checkType(arg)
            end
        }
        if actualRets != "void"
            checkType(actualRets)
        end
    end

    def throwsException
        rets =~ /^exception\//
    end

    def actualRets
        if rets =~ /^exception\//
            $'
        else
            rets
        end
    end

    def nativeReturnType
        if throwsException
            "filc_exception_and_" + underbarType(actualRets)
        else
            rets
        end
    end
end

class OutSignature
    attr_reader :name, :args, :rets

    def initialize(name, args, rets)
        @name = name
        @args = args
        @rets = rets
        args.each {
            | arg |
            checkType(arg)
        }
        if rets != "void"
            checkType(rets)
        end
    end
end

$signatures = []
$defines = []

def addSig(rets, name, *args)
    sig = Signature.new(name, args, rets, $defines.dup)
    $signatures << sig
end

def forMusl
    $defines.push "FILC_MUSL"
    yield
    $defines.pop
end

def forFilBSD
    $defines.push "FILC_FILBSD"
    yield
    $defines.pop
end

$outSignatures = []
def addOutSig(name, rets, *args)
    sig = OutSignature.new(name, args, rets)
    $outSignatures << sig
end

addSig "filc_ptr", "zgc_alloc", "size_t"
addSig "filc_ptr", "zgc_aligned_alloc", "size_t", "size_t"
addSig "filc_ptr", "zgc_realloc", "filc_ptr", "size_t"
addSig "filc_ptr", "zgc_aligned_realloc", "filc_ptr", "size_t", "size_t"
addSig "void", "zgc_free", "filc_ptr"
addSig "filc_ptr", "zgetlower", "filc_ptr"
addSig "filc_ptr", "zgetupper", "filc_ptr"
addSig "bool", "zhasvalidcap", "filc_ptr"
addSig "bool", "zisunset", "filc_ptr"
addSig "bool", "zisint", "filc_ptr"
addSig "int", "zptrphase", "filc_ptr"
addSig "filc_ptr", "zptrtable_new"
addSig "size_t", "zptrtable_encode", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zptrtable_decode", "filc_ptr", "size_t"
addSig "filc_ptr", "zexact_ptrtable_new"
addSig "size_t", "zexact_ptrtable_encode", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zexact_ptrtable_decode", "filc_ptr", "size_t"
addSig "size_t", "ztesting_get_num_ptrtables"
addSig "filc_ptr", "zptr_to_new_string", "filc_ptr"
addSig "void", "zmemset", "filc_ptr", "unsigned", "size_t"
addSig "void", "zmemmove", "filc_ptr", "filc_ptr", "size_t"
addSig "int", "zmemcmp", "filc_ptr", "filc_ptr", "size_t"
addSig "void", "zrun_deferred_global_ctors"
addSig "void", "zprint", "filc_ptr"
addSig "void", "zprint_long", "long"
addSig "void", "zprint_ptr", "filc_ptr"
addSig "size_t", "zstrlen", "filc_ptr"
addSig "int", "zisdigit", "int"
addSig "void", "zerror", "filc_ptr"
addSig "void", "zfence"
addSig "void", "zstore_store_fence"
addSig "void", "zcompiler_fence"
addSig "bool", "zunfenced_weak_cas_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "bool", "zweak_cas_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zunfenced_strong_cas_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zstrong_cas_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zunfenced_xchg_ptr", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zxchg_ptr", "filc_ptr", "filc_ptr"
addSig "void", "zatomic_store_ptr", "filc_ptr", "filc_ptr"
addSig "void", "zunfenced_atomic_store_ptr", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zatomic_load_ptr", "filc_ptr"
addSig "filc_ptr", "zunfenced_atomic_load_ptr", "filc_ptr"
addSig "int", "zpark_if", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "double"
addSig "void", "zunpark_one", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "unsigned", "zunpark", "filc_ptr", "unsigned"
addSig "bool", "zis_runtime_testing_enabled"
addSig "void", "zvalidate_ptr", "filc_ptr"
addSig "void", "zgc_request_and_wait"
addSig "void", "zscavenge_synchronously"
addSig "void", "zscavenger_suspend"
addSig "void", "zscavenger_resume"
addSig "void", "zstack_scan", "filc_ptr", "filc_ptr"
addSig "exception/int", "_Unwind_RaiseException", "filc_ptr"
addSig "void", "zlongjmp", "filc_ptr", "int"
addSig "void", "z_longjmp", "filc_ptr", "int"
addSig "void", "zsiglongjmp", "filc_ptr", "int"
addSig "void", "zmake_setjmp_save_sigmask", "bool"
addSig "void", "zregister_sys_errno_handler", "filc_ptr"
addSig "void", "zregister_sys_dlerror_handler", "filc_ptr"
addSig "int", "zsys_ioctl", "int", "unsigned long", "..."
addSig "ssize_t", "zsys_writev", "int", "filc_ptr", "int"
addSig "ssize_t", "zsys_read", "int", "filc_ptr", "size_t"
addSig "ssize_t", "zsys_readv", "int", "filc_ptr", "int"
addSig "ssize_t", "zsys_write", "int", "filc_ptr", "size_t"
addSig "int", "zsys_close", "int"
addSig "long", "zsys_lseek", "int", "long", "int"
addSig "void", "zsys_exit", "int"
addSig "unsigned", "zsys_getuid"
addSig "unsigned", "zsys_geteuid"
addSig "unsigned", "zsys_getgid"
addSig "unsigned", "zsys_getegid"
addSig "int", "zsys_open", "filc_ptr", "int", "..."
addSig "int", "zsys_getpid"
addSig "int", "zsys_clock_gettime", "int", "filc_ptr"
addSig "int", "zsys_fstatat", "int", "filc_ptr", "filc_ptr", "int"
addSig "int", "zsys_fstat", "int", "filc_ptr"
addSig "int", "zsys_fcntl", "int", "int", "..."
addSig "int", "zsys_sigaction", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_pipe", "filc_ptr"
addSig "int", "zsys_select", "int", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "void", "zsys_sched_yield"
addSig "int", "zsys_socket", "int", "int", "int"
addSig "int", "zsys_setsockopt", "int", "int", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_bind", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_connect", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_getsockname", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getsockopt", "int", "int", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getpeername", "int", "filc_ptr", "filc_ptr"
addSig "ssize_t", "zsys_sendto", "int", "filc_ptr", "size_t", "int", "filc_ptr", "unsigned"
addSig "ssize_t", "zsys_recvfrom", "int", "filc_ptr", "size_t", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getrlimit", "int", "filc_ptr"
addSig "unsigned", "zsys_umask", "unsigned"
addSig "int", "zsys_getitimer", "int", "filc_ptr"
addSig "int", "zsys_setitimer", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_pause"
addSig "int", "zsys_pselect", "int", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getpeereid", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_kill", "int", "int"
addSig "int", "zsys_raise", "int"
addSig "int", "zsys_dup", "int"
addSig "int", "zsys_dup2", "int", "int"
addSig "int", "zsys_sigprocmask", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_chdir", "filc_ptr"
addSig "int", "zsys_fork"
addSig "int", "zsys_waitpid", "int", "filc_ptr", "int"
addSig "int", "zsys_listen", "int", "int"
addSig "int", "zsys_accept", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_socketpair", "int", "int", "int", "filc_ptr"
addSig "int", "zsys_setsid"
addSig "int", "zsys_execve", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getppid"
addSig "int", "zsys_chroot", "filc_ptr"
addSig "int", "zsys_setuid", "unsigned"
addSig "int", "zsys_seteuid", "unsigned"
addSig "int", "zsys_setreuid", "unsigned", "unsigned"
addSig "int", "zsys_setgid", "unsigned"
addSig "int", "zsys_setegid", "unsigned"
addSig "int", "zsys_setregid", "unsigned", "unsigned"
addSig "int", "zsys_nanosleep", "filc_ptr", "filc_ptr"
addSig "long", "zsys_readlink", "filc_ptr", "filc_ptr", "size_t"
addSig "int", "zsys_chown", "filc_ptr", "unsigned", "unsigned"
addSig "int", "zsys_lchown", "filc_ptr", "unsigned", "unsigned"
addSig "ssize_t", "zsys_sendmsg", "int", "filc_ptr", "int"
addSig "ssize_t", "zsys_recvmsg", "int", "filc_ptr", "int"
addSig "int", "zsys_rename", "filc_ptr", "filc_ptr"
addSig "int", "zsys_unlink", "filc_ptr"
addSig "int", "zsys_link", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zsys_mmap", "filc_ptr", "size_t", "int", "int", "int", "long"
addSig "int", "zsys_munmap", "filc_ptr", "size_t"
addSig "int", "zsys_ftruncate", "int", "long"
addSig "filc_ptr", "zsys_getcwd", "filc_ptr", "size_t"
addSig "filc_ptr", "zsys_dlopen", "filc_ptr", "int"
addSig "filc_ptr", "zsys_dlsym", "filc_ptr", "filc_ptr"
addSig "int", "zsys_poll", "filc_ptr", "unsigned long", "int"
addSig "int", "zsys_faccessat", "int", "filc_ptr", "int", "int"
addSig "int", "zsys_sigwait", "filc_ptr", "filc_ptr"
addSig "int", "zsys_fsync", "int"
addSig "int", "zsys_shutdown", "int", "int"
addSig "int", "zsys_rmdir", "filc_ptr"
addSig "int", "zsys_futimens", "int", "filc_ptr"
addSig "int", "zsys_utimensat", "int", "filc_ptr", "filc_ptr", "int"
addSig "int", "zsys_fchown", "int", "unsigned", "unsigned"
addSig "int", "zsys_fchownat", "int", "filc_ptr", "unsigned", "unsigned", "int"
addSig "int", "zsys_fchdir", "int"
addSig "void", "zsys_sync"
addSig "int", "zsys_access", "filc_ptr", "int"
addSig "int", "zsys_symlink", "filc_ptr", "filc_ptr"
addSig "int", "zsys_mprotect", "filc_ptr", "size_t", "int"
addSig "int", "zsys_getgroups", "int", "filc_ptr"
addSig "int", "zsys_getpgrp"
addSig "int", "zsys_getpgid", "int"
addSig "int", "zsys_setpgid", "int", "int"
addSig "long", "zsys_pread", "int", "filc_ptr", "size_t", "long"
addSig "long", "zsys_preadv", "int", "filc_ptr", "int", "long"
addSig "long", "zsys_pwrite", "int", "filc_ptr", "size_t", "long"
addSig "long", "zsys_pwritev", "int", "filc_ptr", "int", "long"
addSig "int", "zsys_getsid", "int"
addSig "int", "zsys_mlock", "filc_ptr", "size_t"
addSig "int", "zsys_munlock", "filc_ptr", "size_t"
addSig "int", "zsys_mlockall", "int"
addSig "int", "zsys_munlockall"
addSig "int", "zsys_sigpending", "filc_ptr"
addSig "int", "zsys_truncate", "filc_ptr", "long"
addSig "int", "zsys_linkat", "int", "filc_ptr", "int", "filc_ptr", "int"
addSig "int", "zsys_chmod", "filc_ptr", "pizlonated_mode_t"
addSig "int", "zsys_lchmod", "filc_ptr", "pizlonated_mode_t"
addSig "int", "zsys_fchmod", "int", "pizlonated_mode_t"
addSig "int", "zsys_mkfifo", "filc_ptr", "pizlonated_mode_t"
addSig "int", "zsys_mkdirat", "int", "filc_ptr", "pizlonated_mode_t"
addSig "int", "zsys_mkdir", "filc_ptr", "pizlonated_mode_t"
addSig "int", "zsys_fchmodat", "int", "filc_ptr", "pizlonated_mode_t", "int"
addSig "int", "zsys_unlinkat", "int", "filc_ptr", "int"
addSig "filc_ptr", "zthread_self"
addSig "unsigned", "zthread_get_id", "filc_ptr"
addSig "unsigned", "zthread_self_id"
addSig "filc_ptr", "zthread_get_cookie", "filc_ptr"
addSig "void", "zthread_set_self_cookie", "filc_ptr"
addSig "filc_ptr", "zthread_create", "filc_ptr", "filc_ptr"
addSig "bool", "zthread_join", "filc_ptr", "filc_ptr"
forMusl {
    addSig "filc_ptr", "zsys_getpwuid", "unsigned"
    addSig "int", "zsys_isatty", "int"
    addSig "int", "zsys_getaddrinfo", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_uname", "filc_ptr"
    addSig "filc_ptr", "zsys_getpwnam", "filc_ptr"
    addSig "int", "zsys_setgroups", "size_t", "filc_ptr"
    addSig "filc_ptr", "zsys_opendir", "filc_ptr"
    addSig "filc_ptr", "zsys_fdopendir", "int"
    addSig "int", "zsys_closedir", "filc_ptr"
    addSig "filc_ptr", "zsys_readdir", "filc_ptr"
    addSig "void", "zsys_rewinddir", "filc_ptr"
    addSig "void", "zsys_seekdir", "filc_ptr", "long"
    addSig "long", "zsys_telldir", "filc_ptr"
    addSig "int", "zsys_dirfd", "filc_ptr"
    addSig "void", "zsys_closelog"
    addSig "void", "zsys_openlog", "filc_ptr", "int", "int"
    addSig "int", "zsys_setlogmask", "int"
    addSig "void", "zsys_syslog", "int", "filc_ptr"
    addSig "int", "zsys_getgrouplist", "filc_ptr", "unsigned", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_initgroups", "filc_ptr", "unsigned"
    addSig "int", "zsys_openpty", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_ttyname_r", "int", "filc_ptr", "size_t"
    addSig "filc_ptr", "zsys_getgrnam", "filc_ptr"
    addSig "void", "zsys_endutxent"
    addSig "filc_ptr", "zsys_getutxent"
    addSig "filc_ptr", "zsys_getutxid", "filc_ptr"
    addSig "filc_ptr", "zsys_getutxline", "filc_ptr"
    addSig "filc_ptr", "zsys_pututxline", "filc_ptr"
    addSig "void", "zsys_setutxent"
    addSig "filc_ptr", "zsys_getlastlogx", "unsigned", "filc_ptr"
    addSig "filc_ptr", "zsys_getlastlogxbyname", "filc_ptr", "filc_ptr"
    addSig "long", "zsys_sysconf_override", "int"
    addSig "int", "zsys_numcores"
    addSig "int", "zsys_getentropy", "filc_ptr", "size_t"
    addSig "int", "zsys_posix_spawn", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr",
           "filc_ptr"
    addSig "int", "zsys_posix_spawnp", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr",
           "filc_ptr"
    addSig "int", "zsys_sendfile", "int", "int", "filc_ptr", "size_t"
}
forFilBSD {
    addSig "int", "zsys_unmount", "filc_ptr", "int"
    addSig "int", "zsys_nmount", "filc_ptr", "unsigned", "int"
    addSig "int", "zsys_chflags", "filc_ptr", "unsigned long"
    addSig "int", "zsys_lchflags", "filc_ptr", "unsigned long"
    addSig "int", "zsys_fchflags", "int", "unsigned long"
    addSig "int", "zsys_chflagsat", "int", "filc_ptr", "unsigned long", "int"
    addSig "int", "zsys_profil", "filc_ptr", "size_t", "unsigned long", "int"
    addSig "int", "zsys_setlogin", "filc_ptr"
    addSig "int", "zsys_acct", "filc_ptr"
    addSig "int", "zsys_reboot", "int"
    addSig "int", "zsys_revoke", "filc_ptr"
    addSig "int", "zsys_ktrace", "filc_ptr", "int", "int", "int"
    addSig "int", "zsys_setgroups", "int", "filc_ptr"
    addSig "int", "zsys_madvise", "filc_ptr", "size_t", "int"
    addSig "int", "zsys_mincore", "filc_ptr", "size_t", "filc_ptr"
    addSig "int", "zsys_swapon", "filc_ptr"
    addSig "int", "zsys_swapoff", "filc_ptr", "unsigned"
    addSig "int", "zsys_getdtablesize"
    addSig "int", "zsys_getpriority", "int", "int"
    addSig "int", "zsys_setpriority", "int", "int", "int"
    addSig "int", "zsys_gettimeofday", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_settimeofday", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_getrusage", "int", "filc_ptr"
    addSig "int", "zsys_flock", "int", "int"
    addSig "int", "zsys_utimes", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_lutimes", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_adjtime", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_quotactl", "filc_ptr", "int", "int", "filc_ptr"
    addSig "int", "zsys_nlm_syscall", "int", "int", "int", "filc_ptr"
    addSig "int", "zsys_nfssvc", "int", "filc_ptr"
    addSig "int", "zsys_rtprio", "int", "int", "filc_ptr"
    addSig "int", "zsys_rtprio_thread", "int", "int", "filc_ptr"
    addSig "int", "zsys_getfh", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_lgetfh", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_getfhat", "int", "filc_ptr", "filc_ptr", "int"
    addSig "int", "zsys_setfib", "int"
    addSig "int", "zsys_ntp_adjtime", "filc_ptr"
    addSig "int", "zsys_ntp_gettime", "filc_ptr"
    addSig "long", "zsys_pathconf", "filc_ptr", "int"
    addSig "long", "zsys_lpathconf", "filc_ptr", "int"
    addSig "long", "zsys_fpathconf", "int", "int"
    addSig "int", "zsys_setrlimit", "int", "filc_ptr"
    addSig "int", "zsys___sysctl", "filc_ptr", "unsigned", "filc_ptr", "filc_ptr", "filc_ptr",
           "size_t"
    addSig "int", "zsys_undelete", "filc_ptr"
    addSig "int", "zsys_semget", "long", "int", "int"
    addSig "int", "zsys_semctl", "int", "int", "int", "..."
    addSig "int", "zsys_semop", "int", "filc_ptr", "size_t"
    addSig "int", "zsys_shmget", "long", "size_t", "int"
    addSig "int", "zsys_shmctl", "int", "int", "filc_ptr"
    addSig "filc_ptr", "zsys_shmat", "int", "filc_ptr", "int"
    addSig "int", "zsys_shmdt", "filc_ptr"
    addSig "int", "zsys_msgget", "long", "int"
    addSig "int", "zsys_msgctl", "int", "int", "filc_ptr"
    addSig "long", "zsys_msgrcv", "int", "filc_ptr", "size_t", "long", "int"
    addSig "int", "zsys_msgsnd", "int", "filc_ptr", "size_t", "int"
    addSig "int", "zsys_futimes", "int", "filc_ptr"
    addSig "int", "zsys_ktimer_create", "int", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_clock_settime", "int", "filc_ptr"
    addSig "int", "zsys_clock_getres", "int", "filc_ptr"
    addSig "int", "zsys_ktimer_delete", "int"
    addSig "int", "zsys_ktimer_gettime", "int", "filc_ptr"
    addSig "int", "zsys_ktimer_getoverrun", "int"
    addSig "int", "zsys_ktimer_settime", "int", "int", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_ffclock_getcounter", "filc_ptr"
    addSig "int", "zsys_ffclock_getestimate", "filc_ptr"
    addSig "int", "zsys_ffclock_setestimate", "filc_ptr"
    addSig "int", "zsys_getcpuclockid2", "long long", "int", "filc_ptr"
    addSig "int", "zsys_minherit", "filc_ptr", "size_t", "int"
    addSig "int", "zsys_issetugid"
    addSig "int", "zsys_aio_read", "filc_ptr"
    addSig "int", "zsys_aio_readv", "filc_ptr"
    addSig "int", "zsys_aio_write", "filc_ptr"
    addSig "int", "zsys_aio_writev", "filc_ptr"
    addSig "int", "zsys_lio_listio", "int", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_aio_return", "filc_ptr"
    addSig "int", "zsys_aio_suspend", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_aio_errr", "filc_ptr"
    addSig "int", "zsys_aio_cancel", "int", "filc_ptr"
    addSig "long", "zsys_aio_waitcomplete", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_aio_fsync", "int", "filc_ptr"
    addSig "int", "zsys_aio_mlock", "filc_ptr"
    addSig "int", "zsys_fhopen", "filc_ptr", "int"
    addSig "int", "zsys_fhstat", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_fhstatfs", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_modnext", "int"
    addSig "int", "zsys_modfnext", "int"
    addSig "int", "zsys_modstat", "int", "filc_ptr"
    addSig "int", "zsys_modfind", "filc_ptr"
    addSig "int", "zsys_kldload", "filc_ptr"
    addSig "int", "zsys_kldunload", "int"
    addSig "int", "zsys_kldunloadf", "int", "int"
    addSig "int", "zsys_kldfind", "filc_ptr"
    addSig "int", "zsys_kldnext", "int"
    addSig "int", "zsys_getresgid", "filc_ptr", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_getresuid", "filc_ptr", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_setresgid", "unsigned", "unsigned", "unsigned"
    addSig "int", "zsys_setresuid", "unsigned", "unsigned", "unsigned"
    addSig "int", "zsys_kldfirstmod", "int"
    addSig "int", "zsys_kldstat", "int", "filc_ptr"
    addSig "int", "zsys___getcwd", "filc_ptr", "size_t"
    addSig "int", "zsys_sched_setparam", "int", "filc_ptr"
    addSig "int", "zsys_sched_getparam", "int", "filc_ptr"
    addSig "int", "zsys_sched_setscheduler", "int", "int", "filc_ptr"
    addSig "int", "zsys_sched_getscheduler", "int"
    addSig "int", "zsys_sched_get_priority_min", "int"
    addSig "int", "zsys_sched_get_priority_max", "int"
    addSig "int", "zsys_sched_rr_get_interval", "int", "filc_ptr"
    addSig "int", "zsys_utrace", "filc_ptr", "size_t"
    addSig "int", "zsys_kldsym", "int", "int", "filc_ptr"
    addSig "int", "zsys_jail", "filc_ptr"
    addSig "int", "zsys_jail_attach", "int"
    addSig "int", "zsys_jail_detach", "int"
    addSig "int", "zsys_jail_get", "filc_ptr", "unsigned", "int"
    addSig "int", "zsys_jail_set", "filc_ptr", "unsigned", "int"
    addSig "int", "zsys___acl_aclcheck_fd", "int", "int", "filc_ptr"
    addSig "int", "zsys___acl_aclcheck_file", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys___acl_aclcheck_link", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys___acl_delete_fd", "int", "int"
    addSig "int", "zsys___acl_delete_file", "filc_ptr", "int"
    addSig "int", "zsys___acl_delete_link", "filc_ptr", "int"
    addSig "int", "zsys___acl_get_fd", "int", "int", "filc_ptr"
    addSig "int", "zsys___acl_get_file", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys___acl_get_link", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys___acl_set_fd", "int", "int", "filc_ptr"
    addSig "int", "zsys___acl_set_file", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys___acl_set_link", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_extattr_delete_fd", "int", "int", "filc_ptr"
    addSig "int", "zsys_extattr_delete_file", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_extattr_delete_link", "filc_ptr", "int", "filc_ptr"
    addSig "long", "zsys_extattr_get_fd", "int", "int", "filc_ptr", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_get_file", "filc_ptr", "int", "filc_ptr", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_get_link", "filc_ptr", "int", "filc_ptr", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_list_fd", "int", "int", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_list_file", "filc_ptr", "int", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_list_link", "filc_ptr", "int", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_set_fd", "int", "int", "filc_ptr", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_set_file", "filc_ptr", "int", "filc_ptr", "filc_ptr", "size_t"
    addSig "long", "zsys_extattr_set_link", "filc_ptr", "int", "filc_ptr", "filc_ptr", "size_t"
    addSig "int", "zsys_extattrctl", "filc_ptr", "int", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_kqueue"
    addSig "int", "zsys_kqueuex", "unsigned"
    addSig "int", "zsys_kevent", "int", "filc_ptr", "int", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys___mac_get_fd", "int", "filc_ptr"
    addSig "int", "zsys___mac_get_file", "filc_ptr", "filc_ptr"
    addSig "int", "zsys___mac_get_link", "filc_ptr", "filc_ptr"
    addSig "int", "zsys___mac_get_pid", "int", "filc_ptr"
    addSig "int", "zsys___mac_get_proc", "filc_ptr"
    addSig "int", "zsys___mac_set_fd", "int", "filc_ptr"
    addSig "int", "zsys___mac_set_file", "filc_ptr", "filc_ptr"
    addSig "int", "zsys___mac_set_link", "filc_ptr", "filc_ptr"
    addSig "int", "zsys___mac_set_proc", "filc_ptr"
    addSig "int", "zsys___mac_execve", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_eaccess", "filc_ptr", "int"
    addSig "int", "zsys_mac_syscall", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_sendfile", "int", "int", "long", "size_t", "filc_ptr", "filc_ptr", "int"
    addSig "int", "zsys_uuidgen", "filc_ptr", "int"
    addSig "int", "zsys_kenv", "int", "filc_ptr", "filc_ptr", "int"
    addSig "int", "zsys___setugid", "int"
    addSig "int", "zsys_ksem_close", "long"
    addSig "int", "zsys_ksem_post", "long"
    addSig "int", "zsys_ksem_wait", "long"
    addSig "int", "zsys_ksem_trywait", "long"
    addSig "int", "zsys_ksem_timedwait", "long", "filc_ptr"
    addSig "int", "zsys_ksem_init", "filc_ptr", "unsigned"
    addSig "int", "zsys_ksem_open", "filc_ptr", "filc_ptr", "int", "unsigned short", "unsigned"
    addSig "int", "zsys_ksem_unlink", "filc_ptr"
    addSig "int", "zsys_ksem_getvalue", "long", "filc_ptr"
    addSig "int", "zsys_ksem_destroy", "long"
    addSig "int", "zsys_audit", "filc_ptr", "int"
    addSig "int", "zsys_auditon", "int", "filc_ptr", "int"
    addSig "int", "zsys_auditctl", "filc_ptr"
    addSig "int", "zsys_getauid", "filc_ptr"
    addSig "int", "zsys_setauid", "filc_ptr"
    addSig "int", "zsys_getaudit", "filc_ptr"
    addSig "int", "zsys_setaudit", "filc_ptr"
    addSig "int", "zsys_getaudit_addr", "filc_ptr", "int"
    addSig "int", "zsys_setaudit_addr", "filc_ptr", "int"
    addSig "int", "zsys_kmq_notify", "int", "filc_ptr"
    addSig "int", "zsys_kmq_open", "filc_ptr", "int", "unsigned short", "filc_ptr"
    addSig "int", "zsys_kmq_setattr", "int", "filc_ptr", "filc_ptr"
    addSig "long", "zsys_kmq_timedreceive", "int", "filc_ptr", "size_t", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_kmq_timedsend", "int", "filc_ptr", "size_t", "unsigned", "filc_ptr"
    addSig "int", "zsys_kmq_unlink", "filc_ptr"
    addSig "int", "zsys__umtx_op", "filc_ptr", "int", "unsigned long", "filc_ptr", "filc_ptr"
    addSig "void", "zsys_abort2", "filc_ptr", "int", "filc_ptr"
    addSig "int", "zsys_futimesat", "int", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_fexecve", "int", "filc_ptr", "filc_ptr"
    addSig "int", "zsys_cpuset_getaffinity", "int", "int", "long long", "size_t", "filc_ptr"
    addSig "int", "zsys_cpuset_setaffinity", "int", "int", "long long", "size_t", "filc_ptr"
    addSig "int", "zsys_cpuset", "filc_ptr"
    addSig "int", "zsys_cpuset_setid", "int", "long long", "int"
    addSig "int", "zsys_cpuset_getid", "int", "int", "long long", "filc_ptr"
}

addOutSig "void_ptr_ptr", "void", "filc_ptr", "filc_ptr"
addOutSig "int_int_ptr", "int", "int", "filc_ptr"
addOutSig "void_int", "void", "int"
addOutSig "void", "void"
addOutSig "bool_ptr_ptr", "bool", "filc_ptr", "filc_ptr"
addOutSig "eh_personality", "int", "int", "int", "unsigned long long", "filc_ptr", "filc_ptr"
addOutSig "void_ptr", "void", "filc_ptr"
addOutSig "ptr_ptr", "filc_ptr", "filc_ptr"
addOutSig "bool_ptr", "bool", "filc_ptr"
addOutSig "void_bool_bool_ptr", "void", "bool", "bool", "filc_ptr"

case ARGV[0]
when "src/libpas/filc_native.h"
    File.open("src/libpas/filc_native.h", "w") {
        | outp |
        outp.puts "/* Generated by generate_pizlonated_forwarders.rb */"
        outp.puts "#ifndef FILC_NATIVE_H"
        outp.puts "#define FILC_NATIVE_H"
        outp.puts "#include \"filc_runtime.h\""
        $signatures.each {
            | signature |
            unless signature.defines.empty?
                outp.puts("#if " + signature.defines.join(" && "))
            end
            outp.print "PAS_API #{signature.nativeReturnType} "
            outp.print "filc_native_#{signature.name}(filc_thread* my_thread"
            unless signature.args.empty?
                outp.print(", " + signature.args.map.with_index {
                               | arg, index |
                               if arg == "..."
                                   "filc_cc_cursor* args_cursor"
                               else
                                   "#{arg} arg#{index}"
                               end
                           }.join(', '))
            end
            outp.puts ");"
            unless signature.defines.empty?
                outp.puts("#endif /* " + signature.defines.join(" && ") + " */")
            end
        }
        $outSignatures.each {
            | signature |
            outp.print "PAS_API #{signature.rets} "
            outp.print "filc_call_user_#{signature.name}(filc_thread* my_thread, "
            outp.print "bool (*target)(PIZLONATED_SIGNATURE)"
            unless signature.args.empty?
                outp.print(", " + signature.args.map.with_index {
                               | arg, index |
                               "#{arg} arg#{index}"
                           }.join(', '))
            end
            outp.puts ");"
        }
        outp.puts "#endif /* FILC_NATIVE_H */"
    }

when "src/libpas/filc_native_forwarders.c"
    File.open("src/libpas/filc_native_forwarders.c", "w") {
        | outp |
        outp.puts "/* Generated by generate_pizlonated_forwarders.rb */"
        outp.puts "#include \"filc_native.h\""
        $signatures.each {
            | signature |
            unless signature.defines.empty?
                outp.puts("#if " + signature.defines.join(" && "))
            end
            outp.puts "static bool native_thunk_#{signature.name}(PIZLONATED_SIGNATURE)"
            outp.puts "{"
            outp.puts "    FILC_DEFINE_RUNTIME_ORIGIN(origin, \"#{signature.name}\", 0);"
            outp.puts "    struct {"
            outp.puts "        FILC_FRAME_BODY;"
            outp.puts "    } actual_frame;"
            outp.puts "    pas_zero_memory(&actual_frame, sizeof(actual_frame));"
            outp.puts "    filc_frame* frame = (filc_frame*)&actual_frame;"
            outp.puts "    frame->origin = &origin;"
            outp.puts "    filc_native_frame native_frame;"
            outp.puts "    filc_push_frame(my_thread, frame);"
            outp.puts "    filc_push_native_frame(my_thread, &native_frame);"
            outp.puts "    filc_cc_cursor args_cursor = filc_cc_cursor_create_begin(args);"
            signature.args.each_with_index {
                | arg, index |
                if arg == "filc_ptr"
                    outp.puts "    filc_ptr arg#{index} = filc_cc_cursor_get_next_ptr(&args_cursor);"
                elsif arg != "..."
                    outp.puts "    #{arg} arg#{index} = "
                    outp.puts "        filc_cc_cursor_get_next_int(&args_cursor, #{arg});"
                end
            }
            case signature.rets
            when "void"
                outp.print "    "
            when "exception/void"
                outp.print "    filc_exception_and_void result = "
            else
                outp.print "    #{signature.nativeReturnType} result = "
            end
            outp.print "filc_native_#{signature.name}(my_thread"
            unless signature.args.empty?
                outp.print(", " + signature.args.map.with_index {
                               | arg, index |
                               if arg == "..."
                                   "&args_cursor"
                               else
                                   "arg#{index}"
                               end
                           }.join(", "))
            end
            outp.puts ");"
            if signature.actualRets == "void"
                outp.puts "    PAS_UNUSED_PARAM(rets);"
            else
                outp.puts "    filc_cc_cursor rets_cursor = filc_cc_cursor_create_begin(rets);"
                if signature.actualRets == "filc_ptr"
                    outp.print "    *filc_cc_cursor_get_next_ptr_ptr(&rets_cursor) = "
                else
                    outp.puts "    *filc_cc_cursor_get_next_int_ptr("
                    outp.print "        &rets_cursor, #{signature.actualRets}) = "
                end
                if signature.throwsException
                    outp.puts "result.value;"
                else
                    outp.puts "result;"
                end
            end
            outp.puts "    filc_pop_native_frame(my_thread, &native_frame);"
            outp.puts "    filc_pop_frame(my_thread, frame);"
            if signature.throwsException
                outp.puts "    return result.has_exception;"
            else
                outp.puts "    return false;"
            end
            outp.puts "}"
            outp.puts "static filc_object function_object_#{signature.name} = {"
            outp.puts "    .lower = native_thunk_#{signature.name},"
            outp.puts "    .upper = (char*)native_thunk_#{signature.name} + FILC_WORD_SIZE,"
            outp.puts "    .flags = FILC_OBJECT_FLAG_GLOBAL | FILC_OBJECT_FLAG_SPECIAL,"
            outp.puts "    .word_types = { FILC_WORD_TYPE_FUNCTION }"
            outp.puts "};"
            outp.puts "filc_ptr pizlonated_#{signature.name}("
            outp.puts "    filc_global_initialization_context* context)"
            outp.puts "{"
            outp.puts "    PAS_UNUSED_PARAM(context);"
            outp.puts "    return filc_ptr_create_with_ptr_and_manual_tracking("
            outp.puts "        &function_object_#{signature.name},"
            outp.puts "        &native_thunk_#{signature.name});"
            outp.puts "}"
            unless signature.defines.empty?
                outp.puts("#endif /* " + signature.defines.join(" && ") + " */")
            end
        }
        $outSignatures.each {
            | signature |
            outp.puts "struct call_user_#{signature.name}_args {"
            signature.args.each_with_index {
                | arg, index |
                outp.puts "    #{arg} arg#{index};"
            }
            outp.puts "};"
            outp.print "#{signature.rets} "
            outp.print "filc_call_user_#{signature.name}(filc_thread* my_thread, "
            outp.print "bool (*target)(PIZLONATED_SIGNATURE)"
            unless signature.args.empty?
                outp.print(", " + signature.args.map.with_index {
                               | arg, index |
                               "#{arg} arg#{index}"
                           }.join(', '))
            end
            outp.puts ")"
            outp.puts "{"
            outp.puts "    size_t args_size = pas_round_up_to_power_of_2("
            outp.puts "        sizeof(struct call_user_#{signature.name}_args), FILC_WORD_SIZE);"
            outp.puts "    size_t num_arg_words = args_size / FILC_WORD_SIZE;"
            outp.puts "    filc_cc_ptr args;"
            outp.puts "    filc_cc_type* args_type = alloca(PAS_OFFSETOF(filc_cc_type, word_types) + "
            outp.puts "                                     num_arg_words * sizeof(filc_word_type));"
            outp.puts "    args.type = args_type;"
            outp.puts "    args_type->num_words = num_arg_words;"
            outp.puts "    size_t index;"
            outp.puts "    for (index = num_arg_words; index--;)"
            outp.puts "        args_type->word_types[index] = FILC_WORD_TYPE_UNSET;"
            outp.puts "    struct call_user_#{signature.name}_args* args_obj = "
            outp.puts "        (struct call_user_#{signature.name}_args*)alloca("
            outp.puts "            sizeof(struct call_user_#{signature.name}_args));"
            outp.puts "    pas_zero_memory(args_obj, sizeof(struct call_user_#{signature.name}_args));"
            outp.puts "    args.base = args_obj;"
            signature.args.each_with_index {
                | arg, index |
                outp.puts "    PAS_ASSERT(sizeof(#{arg}) == alignof(#{arg}));"
                outp.puts "    PAS_ASSERT(pas_is_power_of_2(sizeof(#{arg})));"
                outp.puts "    index = PAS_OFFSETOF(struct call_user_#{signature.name}_args,"
                outp.puts "                         arg#{index}) / FILC_WORD_SIZE;"
                outp.puts "    PAS_ASSERT(index < num_arg_words);"
                if arg == "filc_ptr"
                    outp.puts "    filc_thread_track_object(my_thread, filc_ptr_object(arg#{index}));"
                    wantedType = "FILC_WORD_TYPE_PTR"
                else
                    wantedType = "FILC_WORD_TYPE_INT"
                end
                outp.puts "    PAS_ASSERT(args_type->word_types[index] == FILC_WORD_TYPE_UNSET ||"
                outp.puts "               args_type->word_types[index] == #{wantedType});"
                outp.puts "    args_type->word_types[index] = #{wantedType};"
                outp.puts "    args_obj->arg#{index} = arg#{index};"
            }
            outp.puts "    filc_cc_ptr rets;"
            if signature.rets == "filc_ptr"
                outp.puts "    filc_ptr rets_obj = filc_ptr_forge_null();"
                outp.puts "    rets.type = &filc_ptr_cc_type;"
                outp.puts "    rets.base = &rets_obj;"
            else
                outp.puts "    pas_uint128 rets_obj = 0;"
                if signature.rets == "void"
                    outp.puts "    rets.type = &filc_void_cc_type;"
                else
                    outp.puts "    rets.type = &filc_int_cc_type;"
                end
                outp.puts "    rets.base = &rets_obj;"
            end
            outp.puts "    filc_lock_top_native_frame(my_thread);"
            outp.puts "    PAS_ASSERT(!target(my_thread, args, rets));"
            outp.puts "    filc_unlock_top_native_frame(my_thread);"
            if signature.rets == "filc_ptr"
                outp.puts "    filc_thread_track_object(my_thread, filc_ptr_object(rets_obj));"
                outp.puts "    return rets_obj;"
            elsif signature.rets != "void"
                outp.puts "    return *(#{signature.rets}*)&rets_obj;"
            end
            outp.puts "}"
        }
    }

else
    raise
end


