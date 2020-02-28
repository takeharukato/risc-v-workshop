# -*- mode: makefile-gmake; coding:utf-8 -*-
# OS kernel sample
# Copyright 2019 Takeharu KATO
#
top=../..
include ${top}/Makefile.inc
HOST_CFLAGS += -I.
tools=mkfs.minixfs copyfile.minixfs
minixfs_objects=super.o inode.o zone.o dentry.o
libklib_objects=bswap.o
libminixfs_dir=${top}/fs/minixfs
libklib_dir=${top}/klib
kern_objects=${minixfs_objects} ${libklib_objects}
copyfile_minixfs_objects=utils.o fsimg-pcache.o \
	mkfs-super.o minixfs-path.o minixfs-fops.o ${kern_objects}
mkfs_minixfs_objects=utils.o  fsimg-pcache.o mkfs-super.o minixfs-path.o ${kern_objects}
objects=mkfs.minixfs.o copyfile.minixfs.o \
	${mkfs_minixfs_objects} ${copyfile_minixfs_objects} ${kern_objects}

objects_depends=$(patsubst %.o,%.d,${objects})

all: ${objects_depends} ${tools}

mkfs.minixfs: mkfs.minixfs.o ${mkfs_minixfs_objects}
	${HOST_CC} ${HOST_CFLAGS} -o $@ mkfs.minixfs.o ${mkfs_minixfs_objects}

copyfile.minixfs: copyfile.minixfs.o ${copyfile_minixfs_objects}
	${HOST_CC} ${HOST_CFLAGS} -o $@ copyfile.minixfs.o ${copyfile_minixfs_objects}

gcov:
ifeq ($(CONFIG_PROFILE),y)
	${GCOV} ${GCOV_OPTS} $(patsubst %.o,%.gcda,${objects})
endif

clean: 
	${RM} ${CLEAN_FILES} ${objects} ${tools}

distclean: clean
	${RM} ${DIST_CLEAN_FILES} *.img

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

bswap.o: ${libklib_dir}/bswap.c 
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

bswap.d: ${libklib_dir}/bswap.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$@" $< 

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

inode.d: ${libminixfs_dir}/inode.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$@" $< 

inode.o: ${libminixfs_dir}/inode.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

super.d: ${libminixfs_dir}/super.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$@" $< 

super.o: ${libminixfs_dir}/super.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

zone.d: ${libminixfs_dir}/zone.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$@" $< 

zone.o: ${libminixfs_dir}/zone.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

dentry.d: ${libminixfs_dir}/dentry.c
	${HOST_CC} -E ${HOST_CFLAGS} -MM -MF"$@" $< 

dentry.o: ${libminixfs_dir}/dentry.c
	${HOST_CC} ${HOST_CFLAGS} -c -o $@ $<

-include *.d
