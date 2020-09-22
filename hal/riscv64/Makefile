# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..

include ${top}/Makefile.inc

ASM_OFFSET_CFLAGS = $(shell echo ${CFLAGS}|sed -e 's@-ggdb[0-9]*@@g')
objects = rv64-prepare.o rv64-xchg.o rv64-spinlock.o rv64-sstatus.o rv64-mie.o \
	rv64-sie.o rv64-sip.o rv64-cpuintr.o rv64-backtrace.o rv64-pgtbl.o \
	rv64-bootmap.o rv64-mmu.o rv64-atomic.o rv64-atomic64.o rv64-pframe.o \
	rv64-cpucache.o rv64-vector.o rv64-traps.o rv64-cpuinfo.o \
	rv64-tpreg.o rv64-timer.o rv64-cycle.o rv64-ctxsw.o rv64-stack.o \
	rv64-thread.o rv64-wfi.o rv64-sbicall.o

ifeq ($(CONFIG_HAL),y)
objects += boot.o rv64-dbg-console.o rv64-plic.o rv64-clint.o
endif

ifeq ($(CONFIG_HAL_USE_SBI),y)
objects += rv64-sbi.o
else
objects += rv64-mmode.o rv64-ksbi.o
endif

depends = $(patsubst %.o,%.d,${objects})
ifeq ($(CONFIG_SMP),y)
QEMU_CPU_OPTS = -smp ${CONFIG_CPUS_NR}
else
QEMU_CPU_OPTS = 
endif

ifeq ($(CONFIG_HAL_USE_SBI),y)
HAL_QEMU_BIOS_OPTS = -bios default
else
HAL_QEMU_BIOS_OPTS = -bios none
endif

HAL_QEMU_OPTS = -machine virt ${HAL_QEMU_BIOS_OPTS} ${QEMU_CPU_OPTS} -kernel ${QEMU_KERNEL}

lib=libhal.a

all: kernel.lds ${depends} ${lib}

${lib}: ${objects}
	${RM} $@
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

kernel.lds: kernel.lds.in ${top}/.config
	${RM} $@
	${SED} -e 's|__KHEAP_SIZE__|${CONFIG_HEAP_SIZE}*1024*1024|g' 	             \
	       -e 's|__PAGE_SIZE__|${CONFIG_HAL_PAGE_SIZE}|g' 		             \
	       -e 's|__KSTACK_PAGES__|(1 << ${CONFIG_KSTACK_PAGE_ORDER})|g'	     \
	       -e 's|__ISTACK_PAGES__|(1 << ${CONFIG_ISTACK_PAGE_ORDER})|g'	     \
	       -e 's|__KERN_VMA_BASE__|${CONFIG_HAL_KERN_VMA_BASE}|g'	             \
	       -e 's|__KERN_KERN_ENTRY_ADDRESS__|${CONFIG_HAL_KERN_ENTRY_ADDRESS}|g' \
	       -e 's|__CPUS_NR__|${CONFIG_CPUS_NR}|g'                                \
	$< > $@

run: ${QEMU_KERNEL}
	${QEMU} ${HAL_QEMU_OPTS} ${QEMU_OPTS}

run-debug: ${QEMU_KERNEL}
	${QEMU} ${HAL_QEMU_OPTS} ${QEMU_OPTS} ${QEMU_DBG_OPTS}

clean:
	${RM} ${CLEAN_FILES} ${lib} kernel.lds asm-offset.s

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d


