# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc

objects=tst-spinlock.o tst-atomic.o tst-atomic64.o tst-memset.o tst-vmmap.o tst-pcache.o \
	tst-cpuinfo.o tst-fixed-point.o tst-proc.o tst-thread.o tst-mutex.o tst-bswap.o \
	tst-minixfs.o tst-vfs-path.o tst-vfs-fstbl.o tst-vfs-mount.o tst-vfs-fd.o \
	tst-vfs-tstfs.o 
ifneq ($(CONFIG_HAL),y)
objects += tst-rv64-pgtbl.o tst-irqctrlr.o tst-bsp-stack.o
endif

ifeq ($(CONFIG_HAL),y)
objects += tst-vmcopy.o	tst-vmstrlen.o
endif

lib=libktest.a

all:${lib}

${lib}:${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

run-cov-tests:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean: 
	${RM} ${CLEAN_FILES} ${lib}

distclean: clean
	${RM} ${DIST_CLEAN_FILES}
