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

