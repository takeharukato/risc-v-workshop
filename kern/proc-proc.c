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
#include <kern/sched-if.h>
#include <kern/proc-if.h>
#include <kern/thr-if.h>

static kmem_cache proc_cache; /**< プロセス管理情報のSLABキャッシュ */
static proc_db g_procdb = __PROCDB_INITIALIZER(&g_procdb); /**< プロセス管理ツリー */
static proc   *kern_proc;    /**< カーネルプロセスのプロセス管理情報 */

/** プロセス管理木
 */
static int _procdb_cmp(struct _proc *_key, struct _proc *_ent);
RB_GENERATE_STATIC(_proc_tree, _proc, ent, _procdb_cmp);

/** 
    プロセス管理データベースエントリ比較関数
    @param[in] key 比較対象領域1
    @param[in] ent データベース内の各エントリ
    @retval 正  keyのpidが entのpidより前にある
    @retval 負  keyのpidが entのpidより後にある
    @retval 0   keyのpidが entのpidに等しい
 */
static int 
_procdb_cmp(struct _proc *key, struct _proc *ent){
	
	if ( key->id <= ent->id )
		return 1;

	if ( ent->id <= key->id )
		return -1;

	return 0;	
}

/**
   プロセス管理情報を割り当てる(共通関数)
   @param[out] procp プロセス管理情報返却アドレス
   @retval     0     正常終了
   @retval    -ENOMEM メモリ不足
 */
static int
proc_allocate_common(proc **procp){
	int          rc;
	proc  *new_proc;

	/* プロセス管理情報を割り当てる
	 */
	rc = slab_kmem_cache_alloc(&proc_cache, KMALLOC_NORMAL, (void **)&new_proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	spinlock_init(&new_proc->lock); /* プロセス管理情報のロックを初期化  */
	/* 参照カウンタを初期化(プロセス管理ツリーからの参照分) */
	refcnt_init(&new_proc->refs); 
	queue_init(&new_proc->thrque);  /* スレッドキューの初期化      */
	new_proc->id = 0;          /* PID                              */
	new_proc->text_start = 0;  /* テキスト領域の開始アドレス       */
	new_proc->text_end = 0;    /* テキスト領域の終了アドレス       */
	new_proc->data_start = 0;  /* データ領域の開始アドレス         */
	new_proc->data_end = 0;    /* データ領域の終了アドレス         */
	new_proc->bss_start = 0;   /* BSS領域の開始アドレス            */
	new_proc->bss_end = 0;     /* BSS領域の終了アドレス            */
	new_proc->heap_start = 0;  /* heap領域開始アドレス             */
	new_proc->heap_end = 0;    /* heap領域終了アドレス             */
	new_proc->stack_start = 0; /* スタック開始ページ               */
	new_proc->stack_end = 0;   /* スタック終了アドレス             */
	new_proc->name[0] = '\0';  /* プロセス名を空文字列に初期化する */

	*procp = new_proc;  /* プロセス管理情報を返却 */

	return 0;

error_out:
	return rc;
}

/**
   カーネルプロセス用のプロセス管理情報を割り当てる
 */
static void
init_kernel_process(void){
	int rc;

	/* プロセス管理情報を割り当てる
	 */
	rc = proc_allocate_common(&kern_proc);
	kassert( rc == 0 );

	kern_proc->pgt = hal_refer_kernel_pagetable(); /* カーネルページテーブルを設定 */
	kern_proc->id = PROC_KERN_PID;                 /* プロセスIDを0設定            */

	return ;
}

/**
   カーネルプロセスへの参照を返却する
 */
proc *
proc_kproc_refer(void){

	return kern_proc;  /* カーネルプロセスへの参照を返却する */
}

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
	int             rc;
	proc     *new_proc;
	proc          *res;
	thread        *thr;
	intrflags   iflags;

	/* プロセス管理情報を割り当てる
	 */
	rc = proc_allocate_common(&new_proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	/* ページテーブルを割り当てる
	 */
	rc = pgtbl_alloc_user_pgtbl(&new_proc->pgt);
	if ( rc != 0 ) 
		goto free_proc_out;  
	
	/* ユーザスレッドを生成する
	 */
	rc = thr_thread_create(THR_TID_AUTO, entry, 
	    (void *)truncate_align(HAL_USER_END_ADDR, HAL_STACK_ALIGN_SIZE), 
	    NULL, SCHED_MIN_USER_PRIO, THR_THRFLAGS_USER, &thr);
	if ( rc != 0 ) 
		goto free_pgtbl_out;  
	
	new_proc->stack_start = PAGE_TRUNCATE(HAL_USER_END_ADDR); /* スタック開始ページ   */
	new_proc->stack_end = HAL_USER_END_ADDR;                  /* スタック終了アドレス */
	new_proc->id = thr->id;                                   /* プロセスIDを設定 */

        /* プロセス管理情報をプロセスツリーに登録 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);
	res = RB_INSERT(_proc_tree, &g_procdb.head, new_proc);
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);
	kassert( res == NULL );

	*procp = new_proc;  /* プロセス管理情報を返却 */
	
	return 0;

free_pgtbl_out:
	pgtbl_free_user_pgtbl(new_proc->pgt);  /* ページテーブルを解放する */

free_proc_out:
	slab_kmem_cache_free(new_proc); /* プロセス情報を解放する */

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

	init_kernel_process();  /* カーネルプロセスを生成する */
}
