top=..
include ${top}/Makefile.inc

objects=main.o spinlock.o cpuintr.o page-pfdb.o page-alloc.o page-slab.o vm-pgtbl.o 
#	vm-copy.o
ifneq ($(CONFIG_HAL),y)
objects += ulandpmem.o
endif

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

