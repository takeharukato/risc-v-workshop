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

static kmem_cache                             proc_cache; /**< プロセス管理情報のSLABキャッシュ */
static proc_db g_procdb = __THRDB_INITIALIZER(&g_procdb); /**< プロセス管理ツリー */

/**
 */
int
proc_allocate(pid id, entry_addr entry, void *usp, thr_prio prio, 
	      thr_flags flags, proc **procp){
	int       rc;
	proc   *proc;
	thread  *thr;
	void *kstack;

	if ( ( prio < SCHED_MIN_PRIO ) || ( prio >= SCHED_MAX_PRIO ) )
		return -EINVAL;

	if ( ( flags & THR_THRFLAGS_USER ) 
	    && ( ( prio < SCHED_MIN_USER_PRIO ) || ( prio >= SCHED_MAX_USER_PRIO ) ) )
		return -EINVAL;

	/* スレッド管理情報を割り当てる
	 */
	rc = slab_kmem_cache_alloc(&proc_cache, KMALLOC_NORMAL, (void **)&proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	rc = pgtbl_alloc_user_pgtbl(&proc->pgt);
	if ( rc != 0 ) 
		goto free_proc_out;  
	
	spinlock_init(&proc->lock);  /* プロセス管理情報のロックを初期化 */
	refcnt_init(&proc->refs);    /* 参照カウンタを初期化(プロセス管理ツリーからの参照分) */
	queue_init(&proc->thrque);   /* スレッドキューの初期化 */
	
	return 0;
free_proc_out:
	pgif_free_page(proc);
error_out:
	return rc;
}

void
proc_free(proc *proc){
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
