# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc

subdirs=vfs minixfs

all:

clean: 
	for dir in ${subdirs}; do \
	${MAKE} -C $${dir} clean ;\
	done
	${RM} ${CLEAN_FILES}

distclean:
	for dir in ${subdirs}; do \
	${MAKE} -C $${dir} distclean ;\
	done
	${RM} ${DIST_CLEAN_FILES}
