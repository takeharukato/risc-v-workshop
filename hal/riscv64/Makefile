top=../..
include ${top}/Makefile.inc
UNAME := $(shell uname)

CFLAGS += -I${top}/include
ASM_OFFSET_CFLAGS = $(shell echo ${CFLAGS}|sed -e 's@-ggdb[0-9]*@@g')
objects = 
ifeq ($(CONFIG_HAL),y)
objects += boot.o
endif

lib=libhal.a

all: kernel.lds ${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

kernel.lds: kernel.lds.in ${top}/.config
	${RM} $@
	${SED} -e 's|__KHEAP_SIZE__|${CONFIG_HEAP_SIZE}*1024*1024|g' 	\
	       -e 's|__PAGE_SIZE__|${CONFIG_HAL_PAGE_SIZE}|g' 		\
	       -e 's|__TSTACK_PAGES__|${CONFIG_TSTACK_PAGE_NR}|g'	\
	       -e 's|__ISTACK_PAGES__|${CONFIG_ISTACK_PAGE_NR}|g'	\
	       -e 's|__KERN_VMA_BASE__|${CONFIG_HAL_KERN_VMA_BASE}|g'	\
	$< > $@

clean:
	${RM} *.o ${lib} vector.S asm-offset.s ${top}/include/hal/asm-offset.h

distclean:clean
	${RM} \#* *~
