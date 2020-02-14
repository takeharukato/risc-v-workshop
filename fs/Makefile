# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
subdirs=minixfs

objects=dev-fsimg.o

ifneq ($(CONFIG_HAL),y)
objects +=
endif

depends = $(patsubst %.o,%.d,${objects})

lib=libfs.a

all: ${depends} ${lib} subsystem

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

gcov:
ifeq ($(CONFIG_PROFILE),y)
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} $@;\
	done
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

${fsimg_obj}: fsimg.S 

fsimg.S: fsimg.S.in ${FSIMG}
	${RM} $@
	${SED} -e 's|__fsimg_file__|${FSIMG}|g' 			\
	       -e 's|__PAGE_SIZE__|${CONFIG_HAL_PAGE_SIZE}|g' 		\
	fsimg.S.in > $@

${FSIMG}:
	${DD} if=/dev/zero of=${FSIMG} bs=1M count=${FSSIZE}
	mkfs.mfs $@
	fuse-mfs ${top}/fsimg.img ${top}/rfs
	echo "Hello World" > ${top}/rfs/hello.txt
	${DD} if=/dev/urandom of=${top}/rfs/second-zone.dat bs=1M count=5
	ls -lai ${top}/rfs
	fusermount -u ${top}/rfs

subsystem:
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} ;\
	done

clean:
	${RM} ${CLEAN_FILES} ${lib} ${FSIMG}  fsimg.S
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} $@;\
	done

distclean: clean
	${RM} ${DIST_CLEAN_FILES}
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} $@;\
	done

-include *.d

