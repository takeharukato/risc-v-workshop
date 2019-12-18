/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Preemption/Thread information relevant definitions                */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/kern-if.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>

static kmem_cache thr_cache;  /* スレッド情報のSLABキャッシュ */
/**
   スレッドを生成する共通処理
   @param[in]  attr スレッド属性
   @param[out] thrp スレッドを指し示すポインタのアドレス
   @note staticにする
*/
int
create_thread(thread_attr *attr, thread **thrp){
	int      rc;
	thread *thr;

	rc = slab_kmem_cache_alloc(&thr_cache, KMALLOC_NORMAL, (void **)&thr);
	if ( rc != 0 )
		return -ENOMEM;

	spinlock_init(&thr->lock);
	thr->state = THR_TSTATE_RUNABLE;

	return 0;
}

/**
   スレッド情報管理機構を初期化する
 */
void
thr_init(void){
	int rc;

	rc = slab_kmem_cache_create(&thr_cache, "thread cache", sizeof(thread),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
