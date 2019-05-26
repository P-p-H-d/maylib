# Note: gcc-ar & gcc-ranlib are needed due to -flto magic.
# Link time Optimization needs 2 flags: flto -ffat-lto-objects
AR=gcc-ar cru
CC=gcc
COV=gcov
RANLIB=gcc-ranlib
RM=rm -f
MKDIR=mkdir -p --
RMDIR=rm -rf
CFLAGS=-g -DMAY_WANT_ASSERT
FAST_CFLAGS=-fexceptions -O3 -fomit-frame-pointer -funroll-loops -ffast-math -march=native -ffunction-sections -fdata-sections -static -flto -ffat-lto-objects
XCFLAGS=-Wall -Wmissing-prototypes -Wwrite-strings -W -std=gnu99 -Wno-format

GMP=
MPFR=
PREFIX=/usr/local

####################### INTERNAL ###############################

VERSION=0.7.6

.SUFFIXES: .c .o

TESTS=t-charge.c t-eval.c t-test.c t-ihm.c t-tune.c
SOURCES=const.c dump.c eval.c expand.c heap.c parser.c predicate.c diff.c subs.c num.c cmp.c set.c get.c get_str.c io.c name.c hash.c range.c ifactor.c eval_trig.c kernel.c may-list.c eval_trigh.c approx.c hold.c sqrtsimp.c gcd1.c match.c rewrite.c data.c rectform.c version.c comdenom.c divexact.c lcm1.c get_domain.c degree.c taylor.c divqr.c gcd2.c collect.c polvar.c extension.c texpand.c rationalize.c e-list.c error.c logging.c eval_func.c sqrfree.c transform.c recursive.c bintree.c smod.c ratfactor.c iterator.c e-series.c combine.c normalsign.c copy.c extract.c karatsuba.c antidiff.c gcdex.c partfrac.c e-rootof.c may-thread.c os-specific.c
HEADERS=may.h may-impl.h may-thread.h may-macros.h
DIST=$(SOURCES) $(HEADERS) $(TESTS) Makefile TODO maylib.pdf maylib.texi COPYING.txt COPYING.LESSER.txt

GMP_DIR=$(shell (test -f $(GMP)/include/gmp.h && echo $(GMP)) || (test -f /usr/local/include/gmp.h && echo /usr/local) || (test -f /usr/include/gmp.h && echo /usr))
MPFR_DIR=$(shell (test -f $(MPFR)/include/mpfr.h && echo $(MPFR)) || (test -f $(GMP_DIR)/include/mpfr.h && echo $(GMP_DIR)))
INCLUDES=-I. -I$(MPFR_DIR)/include -I$(GMP_DIR)/include -I$(PREFIX)/include
LIB_MPFR=$(shell (test -f $(MPFR_DIR)/lib/libmpfr.a && echo $(MPFR_DIR)/lib/libmpfr.a) || echo "-L$(MPFR_DIR)/lib -lmpfr")
LIB_GMP=$(shell (test -f $(GMP_DIR)/lib/libgmp.a && echo $(GMP_DIR)/lib/libgmp.a) || echo "-L$(GMP_DIR)/lib -lgmp")
LIBS=$(LIB_MPFR) $(LIB_GMP)

DEFS=-DCC="$(CC)" -DCFLAGS="$(CFLAGS)"
TARGETS=$(TESTS:.c=)
OBJECTS=$(SOURCES:.c=.o)

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(INCLUDES) $(CFLAGS) $(XCFLAGS) $(DEFS) -c $<

t-%: t-%.c libmay.a
	$(CC) $(CPPFLAGS) $(LDFLAGS) $(INCLUDES) $(CFLAGS) -std=gnu99  $< -o $@ libmay.a $(LIBS)

all: t-eval maylib.pdf maylib.info

# Run the test suite
check:  t-test
	@echo "Starting tests..." && ./t-test && echo "All tests passed"

# Run the test suite in Multi-Threading mode
check-mt:
	make CFLAGS="$(CFLAGS) -DMAY_WANT_THREAD" LIBS="$(LIBS) -lpthread" t-test
	@echo "Starting tests..." && ./t-test && echo "All tests passed"

# Run the test suite under address/thread sanitizer (use MT kernel).
check-sanitize:
	make CFLAGS="$(CFLAGS) -DMAY_WANT_THREAD -fsanitize=address,undefined" LIBS="$(LIBS) -lpthread" clean t-test
	@echo "Starting tests with Address and undefined sanitizer..." && ./t-test && echo "All tests passed"
	make CFLAGS="$(CFLAGS) -DMAY_WANT_THREAD -fsanitize=thread -pie -fPIC" LIBS="$(LIBS) -lpthread" clean t-test
	@echo "Starting tests with Thread sanitizer..." && ./t-test && echo "All tests passed"

# Run under Clang static analyser
clang-static-analyser:
	scan-build -v make

# Run a benchmark.
charge: fast t-charge
	@./t-charge

# Run a profile under cachegrind.
cachegrind: fast t-charge
	@valgrind --tool=cachegrind --branch-sim=yes ./t-eval "(1+x+y+z)^10*(1+(x+y+z+1)^10)" -oexpand

# Run the benchmark using perf utility to get most used functions.
perf:	fast t-charge
	perf record ./t-charge
	perf report

# Run the benchmark using perf utility to get statistic of usage.
perf-stat:	fast t-charge
	perf stat -B ./t-charge

makefile-tmp.cflags:
	@echo "Computing the CFLAGS to use..."
	@echo "int x;" > makefile-tmp.c
	@for i in $(FAST_CFLAGS) ; do $(CC) -c makefile-tmp.c $$i 2> /dev/null && echo $$i ; done > makefile-tmp.cflags

# Generate fast version of the library
fast: makefile-tmp.cflags
	make CFLAGS="$(shell cat makefile-tmp.cflags)" clean t-eval t-charge

# Generate fast version of the library with MT support.
fast-mt: makefile-tmp.cflags
	make CFLAGS="$(shell cat makefile-tmp.cflags) -DMAY_WANT_THREAD" LIBS="$(LIBS) -lpthread" clean t-eval t-charge

# Compute coverage of the test suite.
coverage: clean coverage2

coverage2:
	@make CFLAGS="-fprofile-arcs -ftest-coverage -g" check
	@echo "============================== COVERAGE REPORT ================================="
	@for i in $(SOURCES) ; do printf "%20.20s: %s\n" "$$i" "$$( $(COV) $$i 2> /dev/null | grep -i Lines | tail -n 1 ) " ; done
	lcov --capture --directory . --output-file all.info
	genhtml -o coverage all.info
	echo "Report is available in coverage sub-directory."

# Compute coverage of the benchmark.
coverage-charge: clean
	@make CFLAGS="-fprofile-arcs -ftest-coverage -O2" t-charge
	@./t-charge
	lcov --capture --directory . --output-file all.info
	genhtml -o coverage all.info
	echo "Report is available in coverage sub-directory."

help:
	@echo "make [GMP=GMP Install Directory] [MPFR=MPFR Install Directory] [PREFIX=Install Directory]"
	@echo "make target with target can be:"
	@echo " all:        Command Line evaluator"
	@echo " check:      Check MAYLIB consistency"
	@echo " charge:     Check MAYLIB efficiency"
	@echo " coverage:   Check MAYLIB coverage"
	@echo " cachegrind: Chech MAYLIB cache efficiency"
	@echo " perf:       Analyse MAYLIB efficiency"
	@echo " fast:       Generate MAYLIB with Optimized flags"
	@echo " install:    Install MAYLIB in PREFIX [Default: /usr/local ]"
	@echo " clean:      Clean up build and temporary files"

clean:
	$(RM) $(OBJECTS) libmay.a $(TARGETS)
	$(RM) *.s *~
	$(RM) *.gcov *.gcda *.gcno *.bb *.da *.bbg
	$(RM) maylib.aux maylib.cp maylib.cps maylib.dvi maylib.fn maylib.ky maylib.log maylib.pg maylib.toc maylib.tp maylib.vr maylib.info maylib.fns maylib.vrs gmon.out
	$(RM) cachegrind.out.* makefile-tmp.* *.exe *.stackdump all.info perf.data
	$(RMDIR) coverage

libmay.a: $(OBJECTS) may.h may-impl.h
	$(AR) libmay.a $(OBJECTS)
	$(RANLIB) libmay.a

maylib.info:	maylib.texi
	makeinfo maylib.texi -o maylib.info

maylib.pdf:	maylib.texi
	texi2dvi --pdf maylib.texi

maylib.html: maylib.texi
	makeinfo --html --no-split --number-sections maylib.texi

dist: $(DIST)
	$(RMDIR) may-$(VERSION)
	$(MKDIR) may-$(VERSION)
	cp $(DIST) may-$(VERSION)
	7za a -mx=9 may-$(VERSION).7z may-$(VERSION)
	$(RMDIR) may-$(VERSION)

install: may.h maylib.info libmay.a
	$(MKDIR) "$(PREFIX)/lib"
	$(MKDIR) "$(PREFIX)/include"
	$(MKDIR) "$(PREFIX)/info"
	install -c libmay.a "$(PREFIX)/lib/"
	install -c may.h "$(PREFIX)/include/h"
	install -c maylib.info "$(PREFIX)/info/"
