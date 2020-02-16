# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc
HOST_CFLAGS=${OPT_FLAGS} -Wall ${PROF_OPT_FLAGS} ${INC_FLAGS}
HOST_CFLAGS += -I.
minix_libs=${top}/fs/minixfs/libminixfs.a  ${top}/klib/libklib.a
tools=mkfs.minixfs

mkfs_minixfs_objects=utils.o fsimg-pcache.o mkfs-super.o

objects=mkfs.minixfs.o ${mkfs_minixfs_objects}
objects_depends=$(patsubst %.o,%.d,${objects})

all: ${objects_depends} mkfs.minixfs

mkfs.minixfs: mkfs.minixfs.o ${mkfs_minixfs_objects} ${minix_libs}
	${HOST_CC} ${HOST_CFLAGS} -o $@ mkfs.minixfs.o ${mkfs_minixfs_objects} \
					  	       ${minix_libs}

gcov:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean: 
	${RM} ${CLEAN_FILES} ${objects} ${tools}

distclean: clean
	${RM} ${DIST_CLEAN_FILES} *.img

${top}/fs/minixfs/libminixfs.a:
	${MAKE} -C ${dir $@}

${top}/klib/libklib.a:
	${MAKE} -C ${dir $@}

mkfs.minixfs.o: mkfs.minixfs.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

mkfs.minixfs.d: mkfs.minixfs.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

utils.o: utils.c 
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

utils.d: utils.c 
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

fsimg-pcache.o: fsimg-pcache.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

fsimg-pcache.d: fsimg-pcache.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

mkfs-super.o: mkfs-super.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

mkfs-super.d: mkfs-super.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

-include *.d
