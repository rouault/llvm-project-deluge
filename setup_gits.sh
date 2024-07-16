#!/bin/sh
#
# Copyright (c) 2023-2024 Epic Games, Inc. All Rights Reserved.
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

. libpas/common.sh

set -e
set -x

if test $OS = macosx
then
    if test ! -d llvm-project-clean
    then
        (git clone . llvm-project-clean && \
             cd llvm-project-clean && \
             git checkout 6009708b4367171ccdbf4b5905cb6a803753fe18)
    fi
fi

handle_git_with_branch()
{
    remote_path=$1
    local_path=$2
    branch=$3
    if test -d $local_path
    then
        (cd $local_path && git pull --rebase $remote_path $branch)
    else
        git clone $remote_path $local_path
        (cd $local_path && git checkout $branch)
    fi
}

handle_git()
{
    remote_path=$1
    local_path=$2
    if test -d $local_path
    then
        (cd $local_path && git pull --rebase $remote_path)
    else
        git clone $remote_path $local_path
    fi
}

handle_git https://github.com/pizlonator/deluded-musl musl
handle_git_with_branch https://github.com/pizlonator/deluded-musl yolomusl yolomusl

handle_git https://github.com/pizlonator/deluded-zlib-1.3.git zlib-1.3
handle_git https://github.com/pizlonator/deluded-openssl-3.2.0.git openssl-3.2.0
handle_git https://github.com/pizlonator/deluded-curl-8.5.0.git curl-8.5.0
handle_git https://github.com/pizlonator/deluded-openssh-portable.git deluded-openssh-portable
handle_git https://github.com/pizlonator/pizlonated-pcre-8.39.git pcre-8.39
handle_git https://github.com/pizlonator/pizlonated-jpeg-6b.git pizlonated-jpeg-6b
handle_git https://github.com/pizlonator/pizlonated-bzip2.git pizlonated-bzip2
handle_git https://github.com/pizlonator/pizlonated-xz.git pizlonated-xz
handle_git https://github.com/pizlonator/pizlonated-mg.git pizlonated-mg
handle_git https://github.com/pizlonator/pizlonated-sqlite.git pizlonated-sqlite
handle_git https://github.com/pizlonator/pizlonated-cpython.git pizlonated-cpython

