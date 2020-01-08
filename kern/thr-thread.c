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
static thread_db g_thrdb = __THRDB_INITIALIZER(&g_thrdb);  /**< スレッド管理ツリー */

/** スレッド管理情報比較関数
 */
static int _thread_cmp(struct _thread *_key, struct _thread *_ent);
RB_GENERATE_STATIC(_thrdb_tree, _thread, ent, _thread_cmp);

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
	
	if ( key->id < ent->id )
		return 1;

	if ( key->id > ent->id  )
		return -1;

	return 0;	
}

/**
   スレッドの資源を解放する
   @param[in] thr スレッド管理情報
 */
static void
release_thread(thread *thr){
	cpu_id         cpu;
	cpu_info     *cinf;

	kassert( list_not_linked(&thr->link) );  /* レディキューに繋がっていないことを確認 */

	sched_thread_del(thr);  /* レディキューから外す */
	wque_wakeup(&thr->wque, WQUE_DESTROYED);  /* wait待ち中の子スレッドを起こす */
	
	cpu = krn_current_cpu_get();
	cinf = krn_cpuinfo_get(cpu);

	thr->state = THR_TSTATE_EXIT;  /*  終了処理待ちに遷移 */
	ti_set_delay_dispatch(thr->tinfo);  /* ディスパッチ要求をセットする */
	/* TODO: 回収処理を追加する */
	if ( cinf->cur_ti == thr->tinfo ) {
		
		sched_schedule();  /*  自コアのカレントスレッドだった場合はCPUを解放 */
	}
}

/**
   スレッド生成処理
   @param[in]  id      スレッドID
   @param[in]  entry   スレッドエントリアドレス
   @param[in]  usp     ユーザスタックポインタ初期値
   @param[in]  kstktop カーネルスタックの先頭アドレス (NULLの場合は動的に割当てる)
   @param[in]  prio    初期化時のスレッド優先度
   @param[in]  kstack  カーネルスタック
   @param[out] thrp    スレッド管理情報のアドレスの返却先 (NULLを指定すると返却しない)
   @retval     0      正常終了
   @retval    -EINVAL 不正な優先度を指定した
   @retval    -ENOMEM メモリ不足
   @retval    -ENOSPC スレッドIDに空きがない
*/
int
thr_thread_create(tid id, entry_addr entry, void *usp, void *kstktop, thr_prio prio, 
		  thr_flags flags, thread **thrp){
	int            rc;
	tid         newid;
	void      *newstk;
	thread       *thr;
	thread       *res;
	intrflags  iflags;

	if ( ( prio < SCHED_MIN_PRIO ) || ( prio >= SCHED_MAX_PRIO ) )
		return -EINVAL;

	if ( ( flags & THR_THRFLAGS_USER ) 
	    && ( ( prio < SCHED_MIN_USER_PRIO ) || ( prio >= SCHED_MAX_USER_PRIO ) ) )
		return -EINVAL;

	/* スレッド管理情報を割り当てる
	 */
	rc = slab_kmem_cache_alloc(&thr_cache, KMALLOC_NORMAL, (void **)&thr);
	if ( rc != 0 ) {

		rc = -ENOMEM;
		goto error_out;  /* メモリ不足 */
	}


	/* スレッド管理情報の初期化
	 */
	spinlock_init(&thr->lock);  /* スレッド管理情報のロックを初期化 */
	refcnt_init(&thr->refs);    /* 参照カウンタを初期化(スレッド管理ツリーからの参照分) */
	list_init(&thr->link);      /* スケジューラキューへのリストエントリを初期化  */
	thr->flags = flags;         /* スレッドの属性値を設定 */
	/* スレッドを生成したスレッドを親スレッドに設定 */
	thr->parent = ti_get_current_thread(); 
	wque_init_wait_queue(&thr->wque);  /* wait待ちスレッドの待ちキューを初期化  */
	thr->exitcode = 0;                 /* 終了コードを初期化 */
	thr->state = THR_TSTATE_RUNABLE;   /* スレッドを実行可能状態に遷移  */

	thr->attr.ini_prio = prio;   /* 初期化時優先度を初期化 */
	thr->attr.base_prio = prio;  /* ベース優先度を初期化   */
	thr->attr.cur_prio = prio;   /* 現在の優先度を初期化   */

	newstk = kstktop;        /* 指定されたカーネルスタックの先頭アドレスをセットする */
	if ( newstk == NULL ) {  /* スタックを動的に割り当てる場合 */

		rc = pgif_get_free_page_cluster(&newstk, KC_KSTACK_ORDER, 
		    KMALLOC_NORMAL, PAGE_USAGE_KSTACK);  /* カーネルスタックページの割当て */
		if ( rc != 0 ) {

			rc = -ENOMEM;  /* メモリ不足 */
			goto free_thr_out;
		}
		thr->flags |= THR_THRFLAGS_MANAGED_STK; /* ページプールからスタックを割当て */
	}

	thr->attr.kstack_top = newstk;  /* カーネルスタックの先頭アドレスを設定 */
	thr->attr.kstack = newstk;      /* スタック位置をスタックの先頭に初期化 */

	/* スレッドスイッチコンテキスト, 例外コンテキストの初期化
	 */
	hal_setup_thread_context(entry, usp, thr->flags, &thr->attr.kstack);

	/* スレッドIDの割当て, スレッド管理ツリーへの登録
	 */
	/* スレッド管理ツリーのロックを獲得 */
	spinlock_lock_disable_intr(&g_thrdb.lock, &iflags);

	/* スレッドIDを設定
	 */
	if ( THR_TID_RSV_ID_NR > id )
		thr->id = id;
	else {

		newid = bitops_ffc(&g_thrdb.idmap);  /* 空きIDを取得 */
		if ( newid == 0 ) {

			/* スレッド管理ツリーのロックを解放 */
			spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);
			rc = -ENOSPC;  /* 空IDがない */
			goto free_stk_out;
		}
		thr->id = newid - 1;  /* IDを初期化 */
		/* ビットマップ中のビットを使用中にセット */
		bitops_set(thr->id, &g_thrdb.idmap); 
	}

	res = RB_INSERT(_thrdb_tree, &g_thrdb.head, thr);  /* 生成したスレッドを登録 */

	/* スレッド管理ツリーのロックを解放 */
	spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);
	kassert( res == NULL );

	if ( thrp != NULL )
		*thrp = thr;  /* スレッド情報を返却 */

	return 0;

free_stk_out:
	if ( kstktop == NULL )
		pgif_free_page(newstk);  /* 動的に割り当てたスタックを解放 */

free_thr_out:
	slab_kmem_cache_free((void *)thr);  /* スレッド管理情報を解放 */

error_out:
	return rc;
}

/**
   スレッドスイッチ処理
   @param[in] prev CPUを解放するスレッド
   @param[in] next CPUを得るスレッド
 */
void
thr_thread_switch(thread *prev, thread *next){

	/* TODO: ページテーブルを不活性化 (プロセス管理実装後) */
	hal_thread_switch(&prev->attr.kstack, &next->attr.kstack);
	/* TODO: ページテーブルを活性化 (プロセス管理実装後) */
	krn_cpuinfo_update();     /* CPU情報を更新      */
	ti_update_current_cpu();  /* スレッド情報を更新 */
}

/**
   スレッドへの参照を得る
   @param[in] thr スレッド管理情報
   @retval 真 スレッドへの参照を獲得できた
   @retval 偽 スレッドへの参照を獲得できなかった
 */
bool
thr_ref_inc(thread *thr){

	/*  スレッド終了中(スレッド管理ツリーから外れているスレッドの最終参照解放中)でなければ, 
	 *  利用カウンタを加算し, 加算前の値を返す  
	 */
	return ( refcnt_inc_if_valid(&thr->refs) != 0 );  /* 以前の値が0の場合加算できない */
}

/**
   スレッドへの参照を解放する
   @param[in] thr スレッド管理情報
   @retval 真 スレッドの最終参照者だった
   @retval 偽 スレッドの最終参照者でなかった
 */
bool
thr_ref_dec(thread *thr){
	bool           res;
	thread    *thr_res;
	intrflags   iflags;

	/*  スレッドの最終参照者であればスレッドを解放する
	 */
	res = refcnt_dec_and_lock_disable_intr(&thr->refs, &g_thrdb.lock, &iflags);
	if ( res ) {  /* 最終参照者だった場合  */

		/* スレッドをツリーから削除 */
		thr_res = RB_REMOVE(_thrdb_tree, &g_thrdb.head, thr);
		kassert( thr_res != NULL );

		bitops_clr(thr->id, &g_thrdb.idmap);      /* IDを返却 */

		/* スレッド管理ツリーのロックを解放 */
		spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);
		release_thread(thr);  /* スレッドの資源を解放する  */
	}

	return res;
}

/**
   スレッド情報管理機構を初期化する
 */
void
thr_init(void){
	int rc;
	int  i;

	/* スレッド管理情報のキャッシュを初期化する */
	rc = slab_kmem_cache_create(&thr_cache, "thread cache", sizeof(thread),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* スレッドIDビットマップを初期化
	 */
	bitops_zero(&g_thrdb.idmap);
	for(i = 0; THR_TID_RSV_ID_NR > i; ++i) {

		bitops_set(i, &g_thrdb.idmap);  /* 予約IDを使用済みIDに設定 */
	}

}
