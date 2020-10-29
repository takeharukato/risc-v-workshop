# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc

objects=block-device.o bio.o

ifneq ($(CONFIG_HAL),y)
objects +=
endif

depends = $(patsubst %.o,%.d,${objects})

lib=libbdev.a

all: ${depends} ${lib}

${lib}: ${objects}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@
run-cov-tests:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean:
	${RM} ${CLEAN_FILES} ${lib} ${FSIMG}  fsimg.S

distclean: clean
	${RM} ${DIST_CLEAN_FILES}

-include *.d
