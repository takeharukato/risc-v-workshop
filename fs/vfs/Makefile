# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc
subdirs=

objects=vfs-init.o vfs-path.o vfs-fstbl.o vfs-mount.o vfs-fd.o vfs-open.o vfs-close.o \
	vfs-read.o vfs-write.o vfs-create.o vfs-unlink.o vfs-getdents.o \
	vfs-attrhelper.o vfs-setattr.o vfs-getattr.o vfs-lseek.o vfs-mkdir.o \
	vfs-rmdir.o vfs-rename.o vfs-ioctl.o vfs-fsync.o

ifneq ($(CONFIG_HAL),y)
objects +=
endif

depends = $(patsubst %.o,%.d,${objects})

lib=libvfs.a

all: ${depends} ${lib}

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

run-cov-tests:
ifeq ($(CONFIG_PROFILE),y)
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} $@;\
	done
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif


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
