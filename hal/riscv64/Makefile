top=../..

include ${top}/Makefile.inc

ASM_OFFSET_CFLAGS = $(shell echo ${CFLAGS}|sed -e 's@-ggdb[0-9]*@@g')
objects = prepare.o rv64-xchg.o rv64-spinlock.o rv64-sstatus.o rv64-mie.o rv64-sie.o \
	rv64-cpuintr.o rv64-backtrace.o
ifeq ($(CONFIG_HAL),y)
objects += boot.o dbg-console.o
endif

HAL_QEMUOPTS = -machine virt -bios none -kernel ${QEMU_KERNEL}

lib=libhal.a

all: kernel.lds ${lib}

${lib}: ${objects}
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

run: ${QEMU_KERNEL}
	${QEMU} ${HAL_QEMUOPTS} ${QEMU_OPT}

run-debug: ${QEMU_KERNEL}
	${QEMU} ${HAL_QEMUOPTS} ${QEMU_OPT} ${QEMU_DBG_OPT}

clean:
	${RM} *.o ${lib} vector.S asm-offset.s ${top}/include/hal/asm-offset.h

distclean:clean
	${RM} \#* *~
