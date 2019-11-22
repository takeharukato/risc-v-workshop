top=..
include ${top}/Makefile.inc

objects=tst-spinlock.o tst-atomic.o tst-atomic64.o tst-memset.o tst-rv64-pgtbl.o

lib=libktest.a

all:${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
clean:
	${RM} *.o ${lib}

distclean:clean
	${RM} \#* *~
