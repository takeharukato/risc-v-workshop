/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page cache interface for an image file                            */
/*                                                                    */
/**********************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <klib/freestanding.h>
#include <kern/kern-consts.h>
#include <klib/klib-consts.h>
#include <klib/misc.h>
#include <klib/align.h>
#include <klib/compiler.h>
#include <kern/kern-types.h>
#include <kern/page-macros.h>

#include <fs/minixfs/minixfs.h>

#include <minixfs-tools.h>

#include <kern/dev-pcache.h>

#include <utils.h>

static fs_image g_fsimg;
static page_cache  g_pc;

int
pagecache_pagesize(dev_id __unused dev, size_t *pgsizp){
	
	if ( pgsizp != NULL )
		*pgsizp = PAGE_SIZE;

	return 0;
}
int
pagecache_get(dev_id dev, off_t offset, page_cache **pcp){
	off_t addr;

	if ( ( 0 > offset ) || ( offset > g_fsimg.size ) )
		return -EIO;

	addr = truncate_align(offset, PAGE_SIZE);

	memset(&g_pc, 0, sizeof(page_cache));
	g_pc.bdevid = dev;
	g_pc.pgsiz = PAGE_SIZE;
	g_pc.offset = addr;
	g_pc.pc_data = g_fsimg.addr + addr;

	if ( pcp != NULL )
		*pcp = &g_pc;

	return 0;
}

void
pagecache_put(page_cache __unused *pc){

	msync(g_fsimg.addr, g_fsimg.size, MS_SYNC);
}

void
pagecache_mark_dirty(page_cache __unused *pc){

	msync(g_fsimg.addr, g_fsimg.size, MS_SYNC);
}

int
fsimg_create(char *filename, size_t imgsiz){
	int fd;
	int rc;

	fd = open(filename,O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if ( 0 > fd ) {

		fprintf(stderr, "Can not create image-file:%s", filename);
		exit(1);
	}

	imgsiz = roundup_align(imgsiz, PAGE_SIZE);
	rc = ftruncate(fd, imgsiz);
	if ( rc != 0 ){

		fprintf(stderr, "Can not setup image-file:%s len=%lu", filename, 
		    (unsigned long)imgsiz);
		exit(1);
	}

	close(fd);

	return 0;
}

int
fsimg_pagecache_init(char *filename, fs_image **handlep){
	int  rc;
	int  fd;
	void *m;
	struct stat fstat_result;

	fd = safe_open(filename, 0);
	if ( 0 > fd ) {

		fprintf(stderr, "Can not open image-file:%s.\n", filename);
		exit(1);
	}

	rc = fstat(fd, &fstat_result); 	  /*  ファイル記述子が示す対象の属性を得る。 */
	if ( rc < 0 ) {

		fprintf(stderr, "Can not stat image-file:%s.\n", filename);
		exit(1);
	}
	if ( addr_not_aligned(fstat_result.st_size, PAGE_SIZE) ){

		fprintf(stderr, "image-file:%s is not page aligned.\n", filename);
		goto close_fd_out;
	}

	m = mmap(NULL, fstat_result.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if ( m == MAP_FAILED ) {

		fprintf(stderr, "Can not map image-file:%s\n", filename);
		exit(1);
	}

	g_fsimg.file = strdup(filename);
	if ( g_fsimg.file == NULL ){

		fprintf(stderr, "No memory\n");
		exit(1);
	}

	g_fsimg.size = fstat_result.st_size;
	g_fsimg.addr = m;

	close(fd);

	if ( handlep != NULL )
		*handlep = &g_fsimg;

	return 0;
close_fd_out:
	close(fd);
	return rc;
}
