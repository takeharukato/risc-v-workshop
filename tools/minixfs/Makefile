# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc
HOST_CFLAGS += -I.
minix_libs=${top}/fs/minixfs/libminixfs.a  ${top}/klib/libklib.a
tools=mkfs.minixfs copyfile.minixfs

copyfile_minixfs_objects=utils.o fsimg-pcache.o mkfs-super.o minixfs-path.o minixfs-fops.o
mkfs_minixfs_objects=utils.o fsimg-pcache.o mkfs-super.o minixfs-path.o

objects=mkfs.minixfs.o copyfile.minixfs.o ${mkfs_minixfs_objects} ${copyfile_minixfs_objects}
objects_depends=$(patsubst %.o,%.d,${objects})

all: ${objects_depends} ${tools}

mkfs.minixfs: mkfs.minixfs.o ${mkfs_minixfs_objects} ${minix_libs}
	${HOST_CC} ${HOST_CFLAGS} -o $@ mkfs.minixfs.o ${mkfs_minixfs_objects} \
					  	       ${minix_libs}
copyfile.minixfs: copyfile.minixfs.o ${copyfile_minixfs_objects}
	${HOST_CC} ${HOST_CFLAGS} -o $@ copyfile.minixfs.o ${copyfile_minixfs_objects} \
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

copyfile.minixfs.o: copyfile.minixfs.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

copyfile.minixfs.d: copyfile.minixfs.c
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

minixfs-path.o: minixfs-path.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

minixfs-path.d: minixfs-path.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

minixfs-fops.o: minixfs-fops.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

minixfs-fops.d: minixfs-fops.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$(patsubst %.c,%.d, $<)" $< 

-include *.d
