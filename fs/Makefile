# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc

objects=

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

fsimg.S: fsimg.S.in ${FSIMG}
	${RM} $@
	${SED} -e 's|__fsimg_file__|${FSIMG}|g' 	\
	       -e 's|__PAGE_SIZE__|${CONFIG_HAL_PAGE_SIZE}|g' 		\
	fsimg.S.in > $@

${fsimg_obj}: fsimg.S 

${FSIMG}:
	${DD} if=/dev/zero of=${FSIMG} bs=1M count=${FSSIZE}
	${BUSYBOX} mkfs.minix -v ${FSIMG}

clean:
	${RM} ${CLEAN_FILES} ${lib} ${FSIMG}  fsimg.S

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d

