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

# NOTE: This is really assuming *BSD on X86_64. And not just any X86_64, but a recent enough one.

PASCC = clang -march=x86-64-v2 -fPIC -pthread
PASCXX = clang++ -march=x86-64-v2 -fPIC -pthread
FILCC = ../build/bin/clang

all: \
	../$(PREFIXDIR)/lib/libpizlo.so ../$(PREFIXDIR)/lib_test/libpizlo.so \
	../$(PREFIXDIR)/lib/libfilc_crt.so ../$(PREFIXDIR)/lib_test/libfilc_crt.so \
	../$(PREFIXDIR)/lib/libfilc_mincrt.so ../$(PREFIXDIR)/lib_test/libfilc_mincrt.so

include common.mk

../$(PREFIXDIR)/lib/libpizlo.so: $(PASPIZLOOBJS) $(FILPIZLOOBJS)
	clang -shared -o ../$(PREFIXDIR)/lib/libpizlo.so $(PASPIZLOOBJS) $(FILPIZLOOBJS) \
		-pthread -lm -lutil

../$(PREFIXDIR)/lib_test/libpizlo.so: $(PASPIZLOTESTOBJS) $(FILPIZLOOBJS)
	clang -shared -o ../$(PREFIXDIR)/lib_test/libpizlo.so $(PASPIZLOTESTOBJS) $(FILPIZLOOBJS) \
		-pthread -lm -lutil

../$(PREFIXDIR)/lib/libfilc_crt.so: $(MAINOBJS)
	clang -shared -o ../$(PREFIXDIR)/lib/libfilc_crt.so $(MAINOBJS) -pthread

../$(PREFIXDIR)/lib_test/libfilc_crt.so: $(MAINTESTOBJS)
	clang -shared -o ../$(PREFIXDIR)/lib_test/libfilc_crt.so $(MAINTESTOBJS) -pthread

../$(PREFIXDIR)/lib/libfilc_mincrt.so: $(MINMAINOBJS)
	clang -shared -o ../$(PREFIXDIR)/lib/libfilc_mincrt.so $(MINMAINOBJS) -pthread

../$(PREFIXDIR)/lib_test/libfilc_mincrt.so: $(MINMAINTESTOBJS)
	clang -shared -o ../$(PREFIXDIR)/lib_test/libfilc_mincrt.so $(MINMAINTESTOBJS) -pthread

check-pas: build/$(OBJPREFIX)-test_pas
	LD_LIBRARY_PATH=build build/$(OBJPREFIX)-test_pas

build/$(OBJPREFIX)-test_pas: build/lib$(OBJPREFIX)pas.so $(TESTPASOBJS) $(VERIFIEROBJS)
	clang++ -o build/$(OBJPREFIX)-test_pas $(TESTPASOBJS) $(VERIFIEROBJS) \
		-Lbuild -l$(OBJPREFIX)pas -pthread -lutil

build/lib$(OBJPREFIX)pas.so: $(PASTESTOBJS)
	clang -shared -o build/lib$(OBJPREFIX)pas.so $(PASTESTOBJS)

