# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc

all:hal
	${MAKE} -C hal

hal:
	${RM} hal
	${LN} -s ${CPU} hal

clean: hal
	for dir in ${HAL_DIRS} ; do \
	${MAKE} -C $${dir} $@ ;\
	done
	${RM} ${CLEAN_FILES}

distclean: hal clean
	for dir in ${HAL_DIRS} ; do \
	${MAKE} -C $${dir} $@ ;\
	done
	${RM} ${DIST_CLEAN_FILES}
	${RM} hal
