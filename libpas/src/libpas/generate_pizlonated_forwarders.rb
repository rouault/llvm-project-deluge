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
    when 'filc_ptr', 'int', 'unsigned', 'long', 'unsigned long', 'size_t', 'double', 'bool', "ssize_t"
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

$signatureMap = {}
$signatures = []

def addSig(rets, name, *args)
    raise if $signatureMap[name]
    sig = Signature.new(name, args, rets)
    $signatures << sig
    $signatureMap[name] = sig
end

addSig "filc_ptr", "zalloc", "size_t"
addSig "filc_ptr", "zaligned_alloc", "size_t", "size_t"
addSig "filc_ptr", "zrealloc", "filc_ptr", "size_t"
addSig "filc_ptr", "zaligned_realloc", "filc_ptr", "size_t", "size_t"
addSig "void", "zfree", "filc_ptr"
addSig "filc_ptr", "zgetlower", "filc_ptr"
addSig "filc_ptr", "zgetupper", "filc_ptr"
addSig "bool", "zisunset", "filc_ptr"
addSig "bool", "zisint", "filc_ptr"
addSig "int", "zptrphase", "filc_ptr"
addSig "filc_ptr", "zptrtable_new"
addSig "size_t", "zptrtable_encode", "filc_ptr", "filc_ptr"
addSig "filc_ptr", "zptrtable_decode", "filc_ptr", "size_t"
addSig "size_t", "ztesting_get_num_ptrtables"
addSig "filc_ptr", "zptr_to_new_string", "filc_ptr"
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
addSig "filc_ptr", "zsys_getpwuid", "unsigned"
addSig "int", "zsys_sigaction", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_isatty", "int"
addSig "int", "zsys_pipe", "filc_ptr"
addSig "int", "zsys_select", "int", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "void", "zsys_sched_yield"
addSig "int", "zsys_socket", "int", "int", "int"
addSig "int", "zsys_setsockopt", "int", "int", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_bind", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_getaddrinfo", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_connect", "int", "filc_ptr", "unsigned"
addSig "int", "zsys_getsockname", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getsockopt", "int", "int", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getpeername", "int", "filc_ptr", "filc_ptr"
addSig "ssize_t", "zsys_sendto", "int", "filc_ptr", "size_t", "int", "filc_ptr", "unsigned"
addSig "ssize_t", "zsys_recvfrom", "int", "filc_ptr", "size_t", "int", "filc_ptr", "filc_ptr"
addSig "int", "zsys_getrlimit", "int", "filc_ptr"
addSig "unsigned", "zsys_umask", "unsigned"
addSig "int", "zsys_uname", "filc_ptr"
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
addSig "int", "zsys_getgroups", "int", "filc_ptr"
addSig "int", "zsys_getgrouplist", "filc_ptr", "unsigned", "filc_ptr", "filc_ptr"
addSig "int", "zsys_initgroups", "filc_ptr", "unsigned"
addSig "long", "zsys_readlink", "filc_ptr", "filc_ptr", "size_t"
addSig "int", "zsys_openpty", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_ttyname_r", "int", "filc_ptr", "size_t"
addSig "filc_ptr", "zsys_getgrnam", "filc_ptr"
addSig "int", "zsys_chown", "filc_ptr", "unsigned", "unsigned"
addSig "int", "zsys_chmod", "filc_ptr", "unsigned"
addSig "void", "zsys_endutxent"
addSig "filc_ptr", "zsys_getutxent"
addSig "filc_ptr", "zsys_getutxid", "filc_ptr"
addSig "filc_ptr", "zsys_getutxline", "filc_ptr"
addSig "filc_ptr", "zsys_pututxline", "filc_ptr"
addSig "void", "zsys_setutxent"
addSig "filc_ptr", "zsys_getlastlogx", "unsigned", "filc_ptr"
addSig "filc_ptr", "zsys_getlastlogxbyname", "filc_ptr", "filc_ptr"
addSig "ssize_t", "zsys_sendmsg", "int", "filc_ptr", "int"
addSig "ssize_t", "zsys_recvmsg", "int", "filc_ptr", "int"
addSig "int", "zsys_fchmod", "int", "unsigned"
addSig "int", "zsys_rename", "filc_ptr", "filc_ptr"
addSig "int", "zsys_unlink", "filc_ptr"
addSig "int", "zsys_link", "filc_ptr", "filc_ptr"
addSig "long", "zsys_sysconf_override", "int"
addSig "int", "zsys_numcores"
addSig "filc_ptr", "zsys_mmap", "filc_ptr", "size_t", "int", "int", "int", "long"
addSig "int", "zsys_munmap", "filc_ptr", "size_t"
addSig "int", "zsys_ftruncate", "int", "long"
addSig "int", "zsys_getentropy", "filc_ptr", "size_t"
addSig "filc_ptr", "zsys_getcwd", "filc_ptr", "size_t"
addSig "int", "zsys_mkdirat", "int", "filc_ptr", "unsigned"
addSig "filc_ptr", "zsys_dlopen", "filc_ptr", "int"
addSig "filc_ptr", "zsys_dlsym", "filc_ptr", "filc_ptr"
addSig "int", "zsys_poll", "filc_ptr", "unsigned long", "int"
addSig "int", "zsys_faccessat", "int", "filc_ptr", "int", "int"
addSig "int", "zsys_sigwait", "filc_ptr", "filc_ptr"
addSig "int", "zsys_fsync", "int"
addSig "int", "zsys_posix_spawn", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_posix_spawnp", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr", "filc_ptr"
addSig "int", "zsys_shutdown", "int", "int"
addSig "int", "zsys_rmdir", "filc_ptr"
addSig "int", "zsys_futimens", "int", "filc_ptr"
addSig "int", "zsys_fchown", "int", "unsigned", "unsigned"
addSig "filc_ptr", "zthread_self"
addSig "unsigned", "zthread_get_id", "filc_ptr"
addSig "unsigned", "zthread_self_id"
addSig "filc_ptr", "zthread_get_cookie", "filc_ptr"
addSig "void", "zthread_set_self_cookie", "filc_ptr"
addSig "filc_ptr", "zthread_create", "filc_ptr", "filc_ptr"
addSig "bool", "zthread_join", "filc_ptr", "filc_ptr"

case ARGV[0]
when "src/libpas/filc_native.h"
    File.open("src/libpas/filc_native.h", "w") {
        | outp |
        outp.puts "/* Generated by generated_pizlonated_forwarders.rb */"
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
                                   "filc_ptr args"
                               else
                                   "#{arg} arg#{index}"
                               end
                           }.join(', '))
            end
            outp.puts ");"
        }
        outp.puts "#endif /* FILC_NATIVE_H */"
    }

when "src/libpas/filc_native_forwarders.c"
    File.open("src/libpas/filc_native_forwarders.c", "w") {
        | outp |
        outp.puts "/* Generated by generated_pizlonated_forwarders.rb */"
        outp.puts "#include \"filc_native.h\""
        $signatures.each {
            | signature |
            outp.puts "static bool native_thunk_#{signature.name}(PIZLONATED_SIGNATURE)"
            outp.puts "{"
            outp.puts "    FILC_DEFINE_RUNTIME_ORIGIN(origin, \"#{signature.name}\");"
            numObjects = 1
            signature.args.each {
                | arg |
                if arg == 'filc_ptr'
                    numObjects += 1
                end
            }
            outp.puts "    struct {"
            outp.puts "        FILC_FRAME_BODY;"
            outp.puts "        filc_object* objects[#{numObjects}];"
            outp.puts "    } actual_frame;"
            outp.puts "    pas_zero_memory(&actual_frame, sizeof(actual_frame));"
            outp.puts "    filc_frame* frame = (filc_frame*)&actual_frame;"
            outp.puts "    frame->origin = &origin;"
            outp.puts "    frame->num_objects = #{numObjects};"
            outp.puts "    filc_native_frame native_frame;"
            outp.puts "    filc_push_frame(my_thread, frame);"
            outp.puts "    filc_push_native_frame(my_thread, &native_frame);"
            outp.puts "    frame->objects[0] = filc_ptr_object(args);"
            objectCount = 1
            signature.args.each_with_index {
                | arg, index |
                if arg == "filc_ptr"
                    outp.puts "    filc_ptr arg#{index} = filc_ptr_get_next_ptr_with_manual_tracking(&args);"
                    outp.puts "    frame->objects[#{objectCount}] = filc_ptr_object(arg#{index});"
                    objectCount += 1
                elsif arg != "..."
                    outp.puts "    #{arg} arg#{index} = filc_ptr_get_next_#{underbarType(arg)}(&args);"
                end
            }
            case signature.rets
            when "void"
                outp.print "    "
            when "exception/void"
                outp.print "filc_exception_and_void result = "
            else
                if signature.actualRets == "filc_ptr"
                    outp.puts "    filc_check_write_ptr(rets, NULL);"
                else
                    outp.puts "    filc_check_write_int(rets, sizeof(#{signature.actualRets}), NULL);"
                end
                outp.print "    #{signature.nativeReturnType} result = "
            end
            outp.print "filc_native_#{signature.name}(my_thread"
            unless signature.args.empty?
                outp.print(", " + signature.args.map.with_index {
                               | arg, index |
                               if arg == "..."
                                   "args"
                               else
                                   "arg#{index}"
                               end
                           }.join(", "))
            end
            outp.puts ");"
            if signature.actualRets == "void"
                outp.puts "    PAS_UNUSED_PARAM(rets);"
            elsif signature.throwsException
                outp.puts "    *(#{signature.actualRets}*)filc_ptr_ptr(rets) = result.value;"
            else
                outp.puts "    *(#{signature.actualRets}*)filc_ptr_ptr(rets) = result;"
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
            outp.puts "filc_ptr pizlonated_#{signature.name}(filc_global_initialization_context* context)"
            outp.puts "{"
            outp.puts "    PAS_UNUSED_PARAM(context);"
            outp.puts "    return filc_ptr_create_with_ptr_and_manual_tracking("
            outp.puts "        &function_object_#{signature.name},"
            outp.puts "        &native_thunk_#{signature.name});"
            outp.puts "}"
        }
    }

else
    raise
end


