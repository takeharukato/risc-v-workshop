top=..
include ${top}/Makefile.inc
CFLAGS += -I${top}/include
objects=main.o

lib=libklib.a

all:${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
clean:
	${RM} *.o ${lib}

distclean:clean
	${RM} \#* *~
