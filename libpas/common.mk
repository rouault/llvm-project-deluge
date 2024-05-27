PASCFLAGS = -g -O3 -W -Werror -MD
PASCXXFLAGS = -g -O3 -W -Werror -std=c++17 -Wno-unused-parameter -Wno-sign-compare -MD
FILCFLAGS = -O3 -g -W -Werror -Wno-pointer-to-int-cast -MD

PASSRCS = $(sort $(wildcard src/libpas/*.c) src/libpas/filc_native_forwarders.c)
MAINSRCS = $(wildcard ../filc/main/*.c)
FILSRCS = $(wildcard ../filc/src/*.c)
TESTPASSRCS = $(wildcard src/test/*.cpp)
VERIFIERSRCS = $(wildcard src/verifier/*.cpp)

PASPIZLOOBJS = $(patsubst %.c,build/pas-pizlo-release-%.o,$(notdir $(PASSRCS)))
PASPIZLOTESTOBJS = $(patsubst %.c,build/pas-pizlo-test-%.o,$(notdir $(PASSRCS)))
PASTESTOBJS = $(patsubst %.c,build/pas-test-%.o,$(notdir $(PASSRCS)))

MAINOBJS = $(patsubst %.c,build/main-release-%.o,$(notdir $(MAINSRCS)))
MAINTESTOBJS = $(patsubst %.c,build/main-test-%.o,$(notdir $(MAINSRCS)))
MINMAINOBJS = $(patsubst %.c,build/min-main-release-%.o,$(notdir $(MAINSRCS)))
MINMAINTESTOBJS = $(patsubst %.c,build/min-main-test-%.o,$(notdir $(MAINSRCS)))

FILPIZLOOBJS = $(patsubst %.c,build/fil-pizlo-%.o,$(notdir $(FILSRCS)))

TESTPASOBJS = $(patsubst %.cpp,build/test_pas-%.o,$(notdir $(TESTPASSRCS)))
VERIFIEROBJS = $(patsubst %.cpp,build/verifier-%.o,$(notdir $(VERIFIERSRCS)))

GENHEADERS = src/libpas/filc_native.h

-include $(wildcard build/*.d)

clean:
	rm -rf build
	rm -f src/libpas/filc_native_forwarders.c
	rm -f src/libpas/filc_native.h

build/pas-pizlo-release-%.o: src/libpas/%.c $(GENHEADERS)
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DPAS_FILC=1

build/pas-pizlo-test-%.o: src/libpas/%.c $(GENHEADERS)
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DENABLE_PAS_TESTING=1

build/pas-test-%.o: src/libpas/%.c $(GENHEADERS)
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DENABLE_PAS_TESTING=1

build/main-release-%.o: ../filc/main/%.c
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DUSE_LIBC=1 -Isrc/libpas

build/main-test-%.o: ../filc/main/%.c
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DUSE_LIBC=1 -DENABLE_PAS_TESTING=1 \
		-Isrc/libpas

build/min-main-release-%.o: ../filc/main/%.c
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DUSE_LIBC=0 -Isrc/libpas

build/min-main-test-%.o: ../filc/main/%.c
	$(PASCC) $(PASCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DUSE_LIBC=0 -DENABLE_PAS_TESTING=1 \
		-Isrc/libpas

build/fil-pizlo-%.o: ../filc/src/%.c ../build/bin/clang
	$(FILCC) $(FILCFLAGS) -c -o $@ $<

build/test_pas-%.o: src/test/%.cpp
	$(PASCXX) $(PASCXXFLAGS) -c -o $@ $< -DENABLE_PAS_TESTING=1 -Isrc/libpas -Isrc/verifier

build/verifier-%.o: src/verifier/%.cpp
	$(PASCXX) $(PASCXXFLAGS) -c -o $@ $< -DENABLE_PAS_TESTING=1 -Isrc/libpas

src/libpas/filc_native.h: src/libpas/filc_native_forwarders.c
src/libpas/filc_native_forwarders.c: src/libpas/generate_pizlonated_forwarders.rb
	ruby src/libpas/generate_pizlonated_forwarders.rb

