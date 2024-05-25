LEGACYCFLAGS = -g -O3 -W -Werror -MD
FILCFLAGS = -O3 -g -W -Werror -Wno-pointer-to-int-cast -MD

LEGACYSRCS = $(sort $(wildcard src/libpas/*.c) src/libpas/filc_native_forwarders.c)
MAINSRCS = $(wildcard ../filc/main/*.c)
FILSRCS = $(wildcard ../filc/src/*.c)

LEGACYOBJS = $(patsubst %.c,build/release-legacy-%.o,$(notdir $(LEGACYSRCS)))
LEGACYTESTOBJS = $(patsubst %.c,build/test-legacy-%.o,$(notdir $(LEGACYSRCS)))

MAINOBJS = $(patsubst %.c,build/release-main-%.o,$(notdir $(MAINSRCS)))
MAINTESTOBJS = $(patsubst %.c,build/test-main-%.o,$(notdir $(MAINSRCS)))

FILOBJS = $(patsubst %.c,build/fil-%.o,$(notdir $(FILSRCS)))

GENHEADERS = src/libpas/filc_native.h

-include $(wildcard build/*.d)

clean:
	rm -rf build
	rm -f src/libpas/filc_native_forwarders.c
	rm -f src/libpas/filc_native.h

build/release-legacy-%.o: src/libpas/%.c $(GENHEADERS)
	$(LEGACYCC) $(LEGACYCFLAGS) -c -o $@ $< -DPAS_FILC=1

build/test-legacy-%.o: src/libpas/%.c $(GENHEADERS)
	$(LEGACYCC) $(LEGACYCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DENABLE_PAS_TESTING=1

build/release-main-%.o: ../filc/main/%.c
	$(LEGACYCC) $(LEGACYCFLAGS) -c -o $@ $< -DPAS_FILC=1 -Isrc/libpas

build/test-main-%.o: ../filc/main/%.c
	$(LEGACYCC) $(LEGACYCFLAGS) -c -o $@ $< -DPAS_FILC=1 -DENABLE_PAS_TESTING=1 -Isrc/libpas

build/fil-%.o: ../filc/src/%.c ../build/bin/clang $(ALLHEADERS)
	$(FILCC) $(FILCFLAGS) -c -o $@ $<

src/libpas/filc_native.h: src/libpas/filc_native_forwarders.c
src/libpas/filc_native_forwarders.c: src/libpas/generate_pizlonated_forwarders.rb
	ruby src/libpas/generate_pizlonated_forwarders.rb

