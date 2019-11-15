top=..
include ${top}/Makefile.inc

objects=main.o spinlock.o cpuintr.o

depends = $(patsubst %.o,%.d,${objects})

lib=libkern.a

all: ${depends} ${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
clean:
	${RM} *.o *.d ${lib}

distclean:clean
	${RM} \#* *~

-include *.d

