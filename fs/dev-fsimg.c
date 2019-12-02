/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  File system image block device                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/wqueue.h>
#include <kern/mutex.h>
#include <kern/page-if.h>
#include <kern/dev-pcache.h>
#include <kern/fs-fsimg.h>

/**
   FSイメージへのI/O RW
 */
void
fsimg_strategy(page_cache *pc){
	void *sp;

	sp = (void *)((uintptr_t)&_fsimg_start + pc->offset);
	if ( ( (uintptr_t)sp < (uintptr_t)&_fsimg_start ) 
	     && ( (uintptr_t)&_fsimg_end <= (uintptr_t)sp ) ) 
		return;

	if ( PCACHE_IS_DIRTY(pc) )
		memcpy(sp, pc->pc_data, PAGE_SIZE);
	else
		memcpy(pc->pc_data, sp, PAGE_SIZE);
}
/**
   ファイルシステムイメージのロード
 */
void
fsimg_load(void) {
	int             rc;
	off_t          off;
	size_t      fssize;
	page_cache     *pc;

	/*
	 * ファイルシステムイメージ(FSIMG)のページキャッシュを登録
	 */
	fssize=(uintptr_t)&_fsimg_end - (uintptr_t)&_fsimg_start;
	for(off = 0; fssize > off; off += PAGE_SIZE) {

		rc = pagecache_get(FS_FSIMG_DEVID, off,  &pc);
		kassert( rc == 0 );
		kassert( PCACHE_IS_VALID(pc) );
		pagecache_put(pc);
	}
}
