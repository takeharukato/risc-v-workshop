/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Process operations                                                */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/page-if.h>
#include <kern/kern-if.h>
#include <kern/proc-if.h>

static kmem_cache proc_cache; /**< プロセス管理情報のSLABキャッシュ */
static proc_db g_procdb = __THRDB_INITIALIZER(&g_procdb); /**< プロセス管理ツリー */

/**
   ユーザプロセス用のプロセス管理情報を割り当てる
   @param[in]  entry プロセス開始アドレス
   @param[out] procp プロセス管理情報返却アドレス
   @retval     0     正常終了
   @retval    -ENOMEM メモリ不足
   @retval    -ENODEV ミューテックスが破棄された
   @retval    -EINTR  非同期イベントを受信した
   @retval    -EINVAL 不正な優先度を指定した
   @retval    -ENOSPC スレッドIDに空きがない
 */
int
proc_user_allocate(entry_addr entry, proc **procp){
	int       rc;
	proc   *proc;
	thread  *thr;
	void *kstack;

	kassert( flags & THR_THRFLAGS_USER );  /* ユーザプロセスを生成することを確認 */
	
	/* プロセス管理情報を割り当てる
	 */
	rc = slab_kmem_cache_alloc(&proc_cache, KMALLOC_NORMAL, (void **)&proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	/* ページテーブルを割り当てる
	 */
	rc = pgtbl_alloc_user_pgtbl(&proc->pgt);
	if ( rc != 0 ) 
		goto free_proc_out;  
	
	/* ユーザスレッドを生成する
	 */
	rc = thr_thread_create(THR_TID_AUTO, entry, 
	    (void *)truncate_align(HAL_USER_END_ADDR, HAL_STACK_ALIGN_SIZE), 
	    NULL, SCHED_MIN_USER_PRIO, THR_THRFLAGS_USER, &thr);
	if ( rc != 0 ) 
		goto free_pgtbl_out;  
	
	spinlock_init(&proc->lock); /* プロセス管理情報のロックを初期化                     */
	refcnt_init(&proc->refs);   /* 参照カウンタを初期化(プロセス管理ツリーからの参照分) */
	queue_init(&proc->thrque);  /* スレッドキューの初期化 */
	proc->text_start = 0;  /* テキスト領域の開始アドレス  */
	proc->text_end = 0;    /* テキスト領域の終了アドレス  */
	proc->data_start = 0;  /* データ領域の開始アドレス    */
	proc->data_end = 0;    /* データ領域の終了アドレス    */
	proc->bss_start = 0;   /* BSS領域の開始アドレス       */
	proc->bss_end = 0;     /* BSS領域の終了アドレス       */
	proc->heap_start = 0;  /* heap領域開始アドレス        */
	proc->heap_end = 0;    /* heap領域終了アドレス        */
	proc->stack_start = PAGE_TRUNCATE(HAL_USER_END_ADDR); /* スタック開始ページ   */
	proc->stack_end = HAL_USER_END_ADDR;                  /* スタック終了アドレス */
	proc->name[0] = '\0';  /* プロセス名を空文字列に初期化する */

	*procp = proc;  /* プロセス管理情報を返却 */
	
	return 0;

free_pgtbl_out:
	pgtbl_free_user_pgtbl(proc->pgt);  /* ページテーブルを解放する */

free_proc_out:
	slab_kmem_cache_free(proc); /* プロセス情報を解放する */

error_out:
	return rc;
}

/**
   プロセス管理機構を初期化する
 */
void
proc_init(void){
	int rc;

	/* プロセス管理情報のキャッシュを初期化する */
	rc = slab_kmem_cache_create(&proc_cache, "proc cache", sizeof(proc),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
