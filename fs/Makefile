# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc

objects=fsimg.o

ifneq ($(CONFIG_HAL),y)
objects +=
endif

depends = $(patsubst %.o,%.d,${objects})

lib=libfs.a

all: ${depends} ${lib}

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
gcov:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

fsimg.o: fsimg.S ${FSIMG}

${FSIMG}:
	${RM} ${FSIMG}
	${DD} if=/dev/zero of=${FSIMG} bs=1M count=${FSSIZE}
	${BUSYBOX} mkfs.minix -v ${FSIMG}

clean:
	${RM} ${CLEAN_FILES} ${lib}

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d

