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
RB_GENERATE_STATIC(_procdb_tree, _proc, ent, _procdb_cmp);

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
	
	if ( key->id < ent->id )
		return 1;

	if ( key->id > ent->id )
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
allocate_process_common(proc **procp){
	int            rc;
	int             i;
	proc_segment *seg;
	proc    *new_proc;

	/* プロセス管理情報を割り当てる
	 */
	rc = slab_kmem_cache_alloc(&proc_cache, KMALLOC_NORMAL, (void **)&new_proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}

	spinlock_init(&new_proc->lock); /* プロセス管理情報のロックを初期化  */
	/* 参照カウンタを初期化(プロセスの最初のスレッドからの参照分) */
	refcnt_init(&new_proc->refs); 
	queue_init(&new_proc->thrque);  /* スレッドキューの初期化      */
	new_proc->id = 0;          /* PID                              */

	/** セグメントの初期化
	 */
	for( i = 0; PROC_SEG_NR > i; ++i) {

		seg = &new_proc->segments[i];         /* セグメント情報参照     */
		seg->start = 0;             /* セグメント開始アドレス */
		seg->end = 0;               /* セグメント終了アドレス */
		seg->prot = VM_PROT_NONE;   /* 保護属性               */
		seg->flags = VM_FLAGS_NONE; /* マップ属性             */
	}
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
	rc = allocate_process_common(&kern_proc);
	kassert( rc == 0 );

	kern_proc->pgt = hal_refer_kernel_pagetable(); /* カーネルページテーブルを設定 */
	kern_proc->id = PROC_KERN_PID;                 /* プロセスIDを0設定            */

	return ;
}

/**
   ユーザプロセスのセグメントを解放する(内部関数)
   @param[in]  p          プロセス管理情報
   @param[in]  start      アンマップする仮想空間中の開始アドレス
   @param[in]  end        アンマップする仮想空間中の終了アドレス
   @param[in]  flags      ページ割り当て要否の判断に使用するマップ属性
 */
static void
release_process_segment(proc *p, vm_vaddr start, vm_vaddr end, vm_flags flags){
	int                rc;
	vm_vaddr     rm_vaddr;
	vm_paddr     rm_paddr;
	vm_prot       rm_prot;
	vm_flags     rm_flags;
	vm_size     rm_pgsize;
	vm_vaddr    sta_vaddr;
	vm_vaddr    end_vaddr;

	/* アドレスをページ境界にそろえる
	 */
	sta_vaddr = PAGE_TRUNCATE(start); /* 開始仮想アドレス */
	end_vaddr = PAGE_ROUNDUP(end);    /* 終了仮想アドレス */

	/* 領域に割り当てられたマッピングを解放する
	 */
	for( rm_vaddr = sta_vaddr; end_vaddr > rm_vaddr; ) {

		/* 領域のマップ情報を得る
		 */
		rc = hal_pgtbl_extract(p->pgt, rm_vaddr, &rm_paddr, &rm_prot, &rm_flags,
		    &rm_pgsize);
		if ( rc == 0 ) { /* メモリがマップされている場合はマッピングを解放する */
			
			rc = vm_unmap(p->pgt, rm_vaddr, flags, rm_pgsize);
			kassert( rc == 0 );
		}

		rm_vaddr += rm_pgsize; /* 次のページ */
	}

	return ;
}
/**
   ユーザプロセス管理情報を解放する(内部関数)
   @param[in] p プロセス管理情報
 */
static void
free_user_process(proc *p){
	int             i;
	proc_segment *seg;

	kassert( p != kern_proc );  /* カーネルプロセスでないことを確認 */

	/** プロセスのセグメントを解放する
	 */
	for(i = PROC_TEXT_SEG; PROC_SEG_NR > i; ++i) {

		seg = &p->segments[i];         /* セグメント情報参照     */
		/* 領域を開放 */
		release_process_segment(p, seg->start, seg->end, seg->flags);
	}

	/* ページテーブルを解放 */
	pgtbl_free_user_pgtbl(p->pgt);

	thr_id_release(p->id);   /* プロセスIDを返却する   */

	slab_kmem_cache_free(p); /* プロセス情報を解放する */	

	return ;
}

/**
   pidをキーにプロセス管理情報への参照を得る
   @param[in] target 検索対象プロセスのpid
   @return NULL 指定されたpidのプロセスが見つからなかった
   @return 見つかったプロセスのプロセス管理情報
   @note   プロセスへの参照をインクリメントするので返却後に
   proc_ref_dec()を呼び出すこと
 */
proc *
proc_find_by_pid(pid target){
	proc          *p;
	proc         key;
	intrflags iflags;
	
	key.id = target; /* キーとなるpidを設定 */

	/* プロセスDBのロックを獲得 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);

	p = RB_FIND(_procdb_tree, &g_procdb.head, &key); /* プロセス管理情報を検索 */

	if ( p != NULL )
		proc_ref_inc(p);  /* プロセス管理構造への参照をインクリメント */

	/* プロセスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

	return p;  /* プロセス管理情報を返却 */
}
/**
   プロセスのマスタースレッドを取得する
   @param[in] target 検索対象プロセスのpid
   @return NULL 指定されたpidのプロセスが見つからなかった
   @return 見つかったプロセスのマスタースレッド
   @note   スレッドへの参照をインクリメントするので返却後に
   thr_ref_dec()を呼び出すこと
 */
thread *
proc_find_thread(pid target){
	bool         res;
	proc          *p;
	thread      *thr;
	proc         key;
	intrflags iflags;
	
	key.id = target; /* キーとなるpidを設定 */

	/* プロセスDBのロックを獲得 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);

	p = RB_FIND(_procdb_tree, &g_procdb.head, &key); /* プロセス管理情報を検索 */
	if ( p == NULL )
		goto unlock_out;

	res = proc_ref_inc(p);  /* スレッド獲得処理用の参照を獲得 */
	kassert( res );  /* 対象のスレッドが含まれるはずなので参照を獲得できる */

	/* プロセスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

	thr = p->master;        /* マスタースレッド取得     */

	res = thr_ref_inc(thr); /*  スレッドの参照を取得    */
	kassert( res );

	res = proc_ref_dec(p);  /* スレッド獲得処理用の参照を獲得 */
	kassert( !res );  /* 最終参照ではないはず */

	return thr;  /* スレッド管理情報を返却 */

unlock_out:
	/* プロセスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

	return NULL;
}

/**
   プロセスにスレッドを追加する
   @param[in] p   プロセス管理情報
   @param[in] thr スレッド管理情報
   @retval  0        正常終了
   @retval -ENOENT   プロセス削除中
 */
int
proc_add_thread(proc *p, thread *thr){
	intrflags iflags;

	if ( !proc_ref_inc(p) )  /* スレッドからの参照をインクリメント */
		return -ENOENT;  /* 削除中のプロセスにスレッドを追加しようとした */

	/* プロセス管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&p->lock, &iflags);

	queue_add(&p->thrque, &thr->proc_link);  /* スレッドキューに追加      */	
	thr->p = p;  /*  プロセスへのポインタを更新 */

	/* プロセス管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&p->lock, &iflags);

	return  0;
}

/**
   プロセスからスレッドを削除する
   @param[in] p   プロセス管理情報
   @param[in] thr スレッド管理情報
   @retval  真    プロセスの最終スレッドを削除した
   @retval  偽    プロセスにスレッドが残存している
 */
bool
proc_del_thread(proc *p, thread *thr){
	bool          rc;
	bool         res;
	intrflags iflags;

	res = proc_ref_inc(p);  /* スレッド削除処理用の参照を獲得 */
	kassert( res );  /* 削除対象のスレッドが含まれるはずなので参照を獲得できる */

	/* プロセス管理情報のロックを獲得 */
	spinlock_lock_disable_intr(&p->lock, &iflags);

	queue_del(&p->thrque, &thr->proc_link);  /* スレッドキューから削除  */	

	/* プロセス管理情報のロックを解放 */
	spinlock_unlock_restore_intr(&p->lock, &iflags);

	rc = proc_ref_dec(p);  /* スレッド削除に伴う参照のデクリメント */
	kassert( !rc );  /* 上記で参照を得ているので最終参照ではないはず */

	/* プロセスキューが空でなく, 終了したスレッドがマスタースレッドの場合
	   マスタースレッドを更新する
	 */
	if ( !queue_is_empty(&p->thrque) && ( thr == thr->p->master ) )
		p->master = container_of(queue_ref_top( &thr->p->thrque ),
		    thread, proc_link);
	
	rc = proc_ref_dec(p);  /* スレッド削除処理用の参照を解放 */

	return  rc;
}

/**
   カーネルプロセスを参照する
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
	proc_segment  *seg;
	thread        *thr;
	pid        new_pid;
	intrflags   iflags;

	/* プロセス管理情報を割り当てる
	 */
	rc = allocate_process_common(&new_proc);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}
	/** プロセスIDを割り当てる
	 */
	rc = thr_id_alloc(&new_pid);
	if ( rc != 0 )
		goto free_proc_out;
	new_proc->id = new_pid;  /* プロセスIDを設定 */

	/* ページテーブルを割り当てる
	 */
	rc = pgtbl_alloc_user_pgtbl(&new_proc->pgt);
	if ( rc != 0 ) 
		goto free_id_out;  
	
	/* ユーザスタックを割り当てる
	 */
	seg = &new_proc->segments[PROC_STACK_SEG];              /* スタックセグメント参照 */
	seg->start = PAGE_TRUNCATE(HAL_USER_END_ADDR);          /* スタック開始ページ */
	seg->end = PAGE_ROUNDUP(HAL_USER_END_ADDR);             /* スタック終了ページ */
	seg->prot = VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE; /* スタック保護属性   */
	seg->flags = VM_FLAGS_USER;                             /* マップ属性         */
	/* スタックを割り当てる */
	rc = vm_map_userpage(new_proc->pgt, seg->start, seg->prot, seg->flags, 
	    PAGE_SIZE, seg->end - seg->start);
	if ( rc != 0 )
		goto free_pgtbl_out;  /* スタックの割当てに失敗した */

	/* ユーザスレッドを生成する
	 */
	rc = thr_thread_create(THR_TID_AUTO, entry, 
	    (void *)truncate_align(HAL_USER_END_ADDR, HAL_STACK_ALIGN_SIZE), 
	    NULL, SCHED_MIN_USER_PRIO, THR_THRFLAGS_USER, &thr);
	if ( rc != 0 ) 
		goto free_stk_out;  /*  スレッドの生成に失敗した */

	queue_add(&new_proc->thrque, &thr->proc_link);  /* スレッドキューに追加      */
	new_proc->master = thr;                         /* マスタースレッドを設定 */

        /* プロセス管理情報をプロセスツリーに登録 */
	spinlock_lock_disable_intr(&g_procdb.lock, &iflags);
	res = RB_INSERT(_procdb_tree, &g_procdb.head, new_proc);
	spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);
	kassert( res == NULL );

	*procp = new_proc;  /* プロセス管理情報を返却 */
	
	return 0;

free_stk_out:
	release_process_segment(new_proc, 
	    new_proc->segments[PROC_STACK_SEG].start, 
	    new_proc->segments[PROC_STACK_SEG].end,
	    new_proc->segments[PROC_STACK_SEG].flags);  /* スタックセグメントを解放する */

free_pgtbl_out:
	pgtbl_free_user_pgtbl(new_proc->pgt);  /* ページテーブルを解放する */
free_id_out:
	thr_id_release(thr->id);  /*  プロセスIDを返却  */
free_proc_out:
	slab_kmem_cache_free(new_proc); /* プロセス情報を解放する */

error_out:
	return rc;
}

/**
   プロセスへの参照を得る
   @param[in] p  プロセス管理情報
   @retval    真 プロセスへの参照を獲得できた
   @retval    偽 プロセスへの参照を獲得できなかった
 */
bool
proc_ref_inc(proc *p){

	/* プロセス終了中(プロセス管理ツリーから外れているスレッドの最終参照解放中)
	 * でなければ, 利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&p->refs) != 0 );  /* 以前の値が0の場合加算できない */
}

/**
   プロセスへの参照を解放する
   @param[in] p  プロセス管理情報
   @retval 真 プロセスの最終参照者だった
   @retval 偽 プロセスの最終参照者でなかった
   @note LO プロセスDBロック, プロセスのロックの順にロックする
 */
bool
proc_ref_dec(proc *p){
	bool           res;
	proc     *proc_res;
	intrflags   iflags;

	/*  スレッドの最終参照者であればスレッドを解放する
	 */
	res = refcnt_dec_and_lock_disable_intr(&p->refs, &g_procdb.lock, &iflags);
	if ( res ) {  /* 最終参照者だった場合  */

		/* スレッドキューが空であることを確認する
		 */
		spinlock_lock(&p->lock);	
		kassert( queue_is_empty(&p->thrque) ); 
		spinlock_unlock(&p->lock);

		/* プロセスをツリーから削除 */
		proc_res = RB_REMOVE(_procdb_tree, &g_procdb.head, p);
		kassert( proc_res != NULL );

		/* TODO: プロセスキューが空だったらプロセスIDのTIDを返却 */

		/* プロセス管理ツリーのロックを解放 */
		spinlock_unlock_restore_intr(&g_procdb.lock, &iflags);

		free_user_process(p);  /* プロセスの資源を解放する  */
	}

	return res;
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
