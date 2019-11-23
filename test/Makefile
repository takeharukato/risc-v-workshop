top=..
include ${top}/Makefile.inc

objects=tst-spinlock.o tst-atomic.o tst-atomic64.o tst-memset.o tst-vmcopy.o \
	tst-vmstrlen.o
ifneq ($(CONFIG_HAL),y)
objects += tst-rv64-pgtbl.o
endif

lib=libktest.a

all:${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
clean:
	${RM} *.o ${lib}

distclean:clean
	${RM} \#* *~
