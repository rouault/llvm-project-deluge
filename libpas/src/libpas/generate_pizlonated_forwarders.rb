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
         'ssize_t', 'unsigned short', 'unsigned long long', 'long long', 'pas_uint128'
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

def unsignedType(type)
    case type
    when 'filc_ptr', 'unsigned', 'unsigned long', 'size_t', 'double', 'bool',
         'unsigned short', 'unsigned long long', 'pas_uint128'
        type
    when 'int'
        'unsigned'
    when 'long'
        'unsigned long'
    when 'ssize_t'
        'size_t'
    when 'long long'
        'unsigned long long'
    else
        raise "Bad type #{type}"
    end
end

def canonicalArgType(type)
    case type
    when 'filc_ptr', 'double', 'pas_uint128'
        type
    when 'int', 'unsigned', 'long', 'unsigned long', 'size_t', 'bool',
         'ssize_t', 'unsigned short', 'unsigned long long', 'long long'
        'uint64_t'
    else
        raise "Bad type #{type}"
    end
end

class Signature
    attr_reader :name, :args, :rets

    def initialize(name, args, rets)
        @name = name
        @args = args
        @rets = rets

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

def addSig(rets, name, *args)
    sig = Signature.new(name, args, rets)
    $signatures << sig
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
addSig "pas_uint128", "zcall_int", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zcall_ptr", "filc_ptr", "filc_ptr"
addSig "void", "zcall_void", "filc_ptr", "filc_ptr"
addSig "bool", "zis_runtime_testing_enabled"
addSig "void", "zvalidate_ptr", "filc_ptr"
addSig "void", "zgc_request_and_wait"
addSig "bool", "zgc_is_stw"
addSig "void", "zscavenge_synchronously"
addSig "void", "zscavenger_suspend"
addSig "void", "zscavenger_resume"
addSig "void", "zdump_stack"
addSig "void", "zstack_scan", "filc_ptr", "filc_ptr"
addSig "exception/int", "_Unwind_RaiseException", "filc_ptr"
addSig "void", "zlongjmp", "filc_ptr", "int"
addSig "void", "z_longjmp", "filc_ptr", "int"
addSig "void", "zsiglongjmp", "filc_ptr", "int"
addSig "void", "zmake_setjmp_save_sigmask", "bool"
addSig "void", "zcpuid", "unsigned", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "void", "zcpuid_count", "unsigned", "unsigned", "filc_ptr", "filc_ptr", "filc_ptr",
       "filc_ptr"
addSig "unsigned long", "zxgetbv"
addSig "void", "zregister_sys_errno_handler", "filc_ptr"
addSig "void", "zregister_sys_dlerror_handler", "filc_ptr"

addSig "int", "zsys_ioctl", "int", "int", "..."
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
addSig "int", "zsys_accept4", "int", "filc_ptr", "filc_ptr", "int"
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
addSig "int", "zsys_chmod", "filc_ptr", "unsigned"
addSig "int", "zsys_lchmod", "filc_ptr", "unsigned"
addSig "int", "zsys_fchmod", "int", "unsigned"
addSig "int", "zsys_mkfifo", "filc_ptr", "unsigned"
addSig "int", "zsys_mkdirat", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_mkdir", "filc_ptr", "unsigned"
addSig "int", "zsys_fchmodat", "int", "filc_ptr", "unsigned", "int"
addSig "int", "zsys_unlinkat", "int", "filc_ptr", "int"
addSig "int", "zsys_acct", "filc_ptr"
addSig "int", "zsys_setgroups", "size_t", "filc_ptr"
addSig "int", "zsys_madvise", "filc_ptr", "size_t", "int"
addSig "int", "zsys_mincore", "filc_ptr", "size_t", "filc_ptr"
addSig "int", "zsys_getpriority", "int", "int"
addSig "int", "zsys_setpriority", "int", "int", "int"
addSig "int", "zsys_gettimeofday", "filc_ptr", "filc_ptr"
addSig "int", "zsys_settimeofday", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getrusage", "int", "filc_ptr"
addSig "int", "zsys_flock", "int", "int"
addSig "int", "zsys_utimes", "filc_ptr", "filc_ptr"
addSig "int", "zsys_lutimes", "filc_ptr", "filc_ptr"
addSig "int", "zsys_adjtime", "filc_ptr", "filc_ptr"
addSig "long", "zsys_pathconf", "filc_ptr", "int"
addSig "long", "zsys_fpathconf", "int", "int"
addSig "int", "zsys_setrlimit", "int", "filc_ptr"
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
addSig "int", "zsys_futimesat", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_clock_settime", "int", "filc_ptr"
addSig "int", "zsys_clock_getres", "int", "filc_ptr"
addSig "int", "zsys_issetugid"
addSig "int", "zsys_getresgid", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getresuid", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_setresgid", "unsigned", "unsigned", "unsigned"
addSig "int", "zsys_setresuid", "unsigned", "unsigned", "unsigned"
addSig "int", "zsys_sched_setparam", "int", "filc_ptr"
addSig "int", "zsys_sched_getparam", "int", "filc_ptr"
addSig "int", "zsys_sched_setscheduler", "int", "int", "filc_ptr"
addSig "int", "zsys_sched_getscheduler", "int"
addSig "int", "zsys_sched_get_priority_min", "int"
addSig "int", "zsys_sched_get_priority_max", "int"
addSig "int", "zsys_sched_rr_get_interval", "int", "filc_ptr"
addSig "int", "zsys_eaccess", "filc_ptr", "int"
addSig "int", "zsys_fexecve", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_isatty", "int"
addSig "int", "zsys_uname", "filc_ptr"
addSig "int", "zsys_sendfile", "int", "int", "filc_ptr", "size_t"
addSig "void", "zsys_futex_wake", "filc_ptr", "int", "int"
addSig "void", "zsys_futex_wait", "filc_ptr", "int", "int"
addSig "int", "zsys_futex_timedwait", "filc_ptr", "int", "int", "filc_ptr", "int"
addSig "int", "zsys_futex_unlock_pi", "filc_ptr", "int"
addSig "int", "zsys_futex_lock_pi", "filc_ptr", "int", "filc_ptr"
addSig "void", "zsys_futex_requeue", "filc_ptr", "int", "int", "int", "filc_ptr"
addSig "int", "zsys_getdents", "int", "filc_ptr", "size_t"
addSig "long", "zsys_getrandom", "filc_ptr", "size_t", "unsigned"
addSig "int", "zsys_epoll_create1", "int"
addSig "int", "zsys_epoll_ctl", "int", "int", "int", "filc_ptr"
addSig "int", "zsys_epoll_wait", "int", "filc_ptr", "int", "int"
addSig "int", "zsys_epoll_pwait", "int", "filc_ptr", "int", "int", "filc_ptr"
addSig "int", "zsys_sysinfo", "filc_ptr"
addSig "int", "zsys_sched_getaffinity", "int", "size_t", "filc_ptr"
addSig "int", "zsys_sched_setaffinity", "int", "size_t", "filc_ptr"
addSig "int", "zsys_posix_fadvise", "int", "long", "long", "int"
addSig "int", "zsys_ppoll", "filc_ptr", "unsigned long", "filc_ptr", "filc_ptr"
addSig "int", "zsys_wait4", "int", "filc_ptr", "int", "filc_ptr"
addSig "int", "zsys_sigsuspend", "filc_ptr"
addSig "int", "zsys_prctl", "int", "..."

addSig "filc_ptr", "zthread_self"
addSig "unsigned", "zthread_get_id", "filc_ptr"
addSig "unsigned", "zthread_self_id"
addSig "filc_ptr", "zthread_get_cookie", "filc_ptr"
addSig "void", "zthread_set_self_cookie", "filc_ptr"
addSig "filc_ptr", "zthread_create", "filc_ptr", "filc_ptr"
addSig "void", "zthread_exit", "filc_ptr"
addSig "bool", "zthread_join", "filc_ptr", "filc_ptr"

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
addOutSig "libc_start_main", "void", "filc_ptr", "int", "filc_ptr", "filc_ptr", "filc_ptr"

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
                if arg != "..."
                    outp.puts "    #{arg} arg#{index} = "
                    outp.puts "        filc_cc_cursor_get_next_#{underbarType(arg)}(&args_cursor);"
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
                outp.puts "    filc_cc_cursor_set_next_#{underbarType(signature.actualRets)}("
                outp.print "        &rets_cursor, "
                if signature.throwsException
                    outp.puts "result.value);"
                else
                    outp.puts "result);"
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
        }
        $outSignatures.each {
            | signature |
            outp.puts "struct call_user_#{signature.name}_args {"
            signature.args.each_with_index {
                | arg, index |
                outp.puts "    #{canonicalArgType(arg)} arg#{index};"
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
                outp.puts "    PAS_ASSERT(sizeof(#{canonicalArgType(arg)})"
                outp.puts "               == alignof(#{canonicalArgType(arg)}));"
                outp.puts "    PAS_ASSERT(pas_is_power_of_2(sizeof(#{canonicalArgType(arg)})));"
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
                outp.print "    args_obj->arg#{index} = "
                if unsignedType(arg) != arg
                    outp.print "(#{unsignedType(arg)})"
                end
                outp.puts "arg#{index};"
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
                outp.puts "    return (#{signature.rets})"
                outp.puts "        *(#{canonicalArgType(signature.rets)}*)&rets_obj;"
            end
            outp.puts "}"
        }
    }

else
    raise
end


