top=../..

include ${top}/Makefile.inc

ASM_OFFSET_CFLAGS = $(shell echo ${CFLAGS}|sed -e 's@-ggdb[0-9]*@@g')
objects = rv64-prepare.o rv64-xchg.o rv64-spinlock.o rv64-sstatus.o rv64-mie.o rv64-sie.o \
	rv64-cpuintr.o rv64-backtrace.o rv64-pgtbl.o rv64-bootmap.o rv64-mmu.o \
	rv64-atomic.o rv64-atomic64.o rv64-pframe.o rv64-cpucache.o rv64-mmode.o \
	rv64-vector.o rv64-traps.o
ifeq ($(CONFIG_HAL),y)
objects += boot.o rv64-dbg-console.o 
endif

depends = $(patsubst %.o,%.d,${objects})
ifeq ($(CONFIG_SMP),y)
QEMU_CPU_OPTS = -smp ${CONFIG_HAL_CPUS_NR}
else
QEMU_CPU_OPTS = 
endif
HAL_QEMUOPTS = -machine virt -bios none ${QEMU_CPU_OPTS} -kernel ${QEMU_KERNEL}
lib=libhal.a

all: kernel.lds ${depends} ${lib}

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

kernel.lds: kernel.lds.in ${top}/.config
	${RM} $@
	${SED} -e 's|__KHEAP_SIZE__|${CONFIG_HEAP_SIZE}*1024*1024|g' 	\
	       -e 's|__PAGE_SIZE__|${CONFIG_HAL_PAGE_SIZE}|g' 		\
	       -e 's|__TSTACK_PAGES__|${CONFIG_TSTACK_PAGE_NR}|g'	\
	       -e 's|__KSTACK_PAGES__|${CONFIG_KSTACK_PAGE_NR}|g'	\
	       -e 's|__ISTACK_PAGES__|${CONFIG_ISTACK_PAGE_NR}|g'	\
	       -e 's|__KERN_VMA_BASE__|${CONFIG_HAL_KERN_VMA_BASE}|g'	\
	       -e 's|__CPUS_NR__|${CONFIG_CPUS_NR}|g'         \
	$< > $@

run: ${QEMU_KERNEL}
	${QEMU} ${HAL_QEMUOPTS} ${QEMU_OPT}

run-debug: ${QEMU_KERNEL}
	${QEMU} ${HAL_QEMUOPTS} ${QEMU_OPT} ${QEMU_DBG_OPT}

clean:
	${RM} ${CLEAN_FILES} ${lib} kernel.lds asm-offset.s

distclean:clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d


