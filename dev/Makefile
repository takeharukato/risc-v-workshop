# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
subdirs=bdev md

objects=

ifneq ($(CONFIG_HAL),y)
objects +=
endif

depends = $(patsubst %.o,%.d,${objects})

lib=

all: ${depends} ${lib} subsystem

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

run-cov-tests:
ifeq ($(CONFIG_PROFILE),y)
	for dir in ${subdirs} ; do \
	${MAKE} -C $${dir} $@;\
	done
endif

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
