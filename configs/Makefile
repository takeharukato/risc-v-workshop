# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=..
include ${top}/Makefile.inc
objects=

all:

hal:
	${RM} hal
	${LN} -s ${CPU} hal

clean:
	${RM} ${CLEAN_FILES}

distclean:clean
	${RM} ${DIST_CLEAN_FILES} hal

