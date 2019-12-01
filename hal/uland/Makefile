top=../..

include ${top}/Makefile.inc

ASM_OFFSET_CFLAGS = $(shell echo ${CFLAGS}|sed -e 's@-ggdb[0-9]*@@g')
klib_objs = x64-atomic.o x64-atomic64.o x64-xchg.o x64-backtrace.o
objects = ${klib_objs} x64-cpucache.o x64-cpuintr.o x64-spinlock.o x64-rflags.o \
	x64-cpuinfo.o x64-prepare.o

ifeq ($(CONFIG_HAL),y)
objects += 
else
objects += x64-pframe.o
endif

depends = $(patsubst %.o,%.d,${objects})
lib=libhal.a

all: ${depends} ${lib}

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

run: 
	${top}/kernel-dbg.elf

gcov:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean:
	${RM} ${CLEAN_FILES} ${lib} asm-offset.s

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d


