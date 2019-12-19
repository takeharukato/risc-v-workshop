/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Thread operations                                                 */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/kern-if.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>

static kmem_cache thr_cache;  /**< スレッド管理情報のSLABキャッシュ */
static thread_db g_thrdb = __THRDB_INITIALIZER(&g_thrdb);  /**< Thread DB */

/** スレッド管理情報比較関数
 */
static int _thread_cmp(struct _thread *_key, struct _thread *_ent);
RB_GENERATE_STATIC(_threaddb_tree, _thread, ent, _thread_cmp);

/** 
    スレッド管理情報比較関数
    @param[in] key 比較対象1
    @param[in] ent RB木内の各エントリ
    @retval 正  keyのスレッドIDがentより前にある
    @retval 負  keyのスレッドIDがentより後にある
    @retval 0   keyのスレッドIDがentに等しい
 */
static int 
_thread_cmp(struct _thread *key, struct _thread *ent){
	
	if ( key->tid < ent->tid )
		return 1;

	if ( key->tid > ent->tid  )
		return -1;

	return 0;	
}

/**
   スレッド生成共通処理 (内部関数)
   @param[in]  attr スレッド属性
   @param[out] thrp スレッドを指し示すポインタのアドレス
   @note staticにする
*/
static int
create_thread(thread_attr *attr, thread **thrp){
	int      rc;
	thread *thr;

	rc = slab_kmem_cache_alloc(&thr_cache, KMALLOC_NORMAL, (void **)&thr);
	if ( rc != 0 )
		return -ENOMEM;

	/*
	 * パラメタ
	 */
	spinlock_init(&thr->lock);
	thr->state = THR_TSTATE_RUNABLE;
	refcnt_init(&thr->refs);
	list_init(&thr->link);

	rc = pgif_get_free_page_cluster(&thr->attr.kstack, KMALLOC_NORMAL, PAGE_USAGE_KSTACK);
	if ( rc != 0 )
		return -ENOMEM;
	
	return 0;
}
/**
   スレッド生成共通処理 (内部関数)
   @param[in]  attr スレッド属性
*/
int
create_kernel_thread(thread_attr *attr){
	int      rc;
	thread *thr;

	create_thread(attr, &thr);
	return 0;
}

/**
   スレッド情報管理機構を初期化する
 */
void
thr_init(void){
	int rc;

	/* スレッド管理情報のキャッシュを初期化する */
	rc = slab_kmem_cache_create(&thr_cache, "thread cache", sizeof(thread),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
