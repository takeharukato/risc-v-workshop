top=..
include ${top}/Makefile.inc

objects=main.o spinlock.o cpuintr.o page-pfdb.o page-alloc.o page-slab.o vm-pgtbl.o \
	vm-copy.o vm-map.o wqueue.o mutex.o
ifneq ($(CONFIG_HAL),y)
objects += ulandpmem.o
endif

depends = $(patsubst %.o,%.d,${objects})

lib=libkern.a

all: ${depends} ${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
gcov:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean:
	${RM} ${CLEAN_FILES} ${lib}

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d

