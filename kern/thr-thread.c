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
#include <kern/proc-if.h>
#include <kern/thr-if.h>
#include <kern/sched-if.h>

static kmem_cache thr_cache;  /**< スレッド管理情報のSLABキャッシュ */
static thread_db  g_thrdb = __THRDB_INITIALIZER(&g_thrdb);  /**< スレッド管理ツリー */

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
   スレッド生成共通処理 (内部関数)
   @param[in]  id      スレッドID
   @param[in]  entry   スレッドエントリアドレス
   @param[in]  usp     ユーザスタックポインタ初期値
   @param[in]  kstktop カーネルスタックの先頭アドレス (NULLの場合は動的に割当てる)
   @param[in]  prio    初期化時のスレッド優先度
   @param[in]  flags   スレッド属性フラグ
   @param[out] thrp    スレッド管理情報のアドレスの返却先 (NULLを指定すると返却しない)
   @retval     0      正常終了
   @retval    -EINVAL 不正な優先度を指定した
   @retval    -ENOMEM メモリ不足
   @retval    -ENOSPC スレッドIDに空きがない
   @note      アイドルスレッド生成処理と通常のスレッド生成処理との共通部分を実装.
   アイドルスレッド生成時は, 親スレッドがいないので親スレッドの参照獲得処理を除いている.
*/
static int
create_thread_common(tid id, entry_addr entry, void *usp, void *kstktop, thr_prio prio, 
		  thr_flags flags, thread **thrp){
	int            rc;
	tid         newid;
	void      *newstk;
	thread       *thr;
	thread       *res;
	intrflags  iflags;

	if ( !SCHED_VALID_PRIO(prio) )
		return -EINVAL;

	if ( ( flags & THR_THRFLAGS_USER ) && ( !SCHED_VALID_USER_PRIO(prio) ) )
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
	spinlock_init(&thr->lock);         /* スレッド管理情報のロックを初期化 */
	thr->state = THR_TSTATE_DORMANT;   /* スレッドを実行可能状態に遷移     */
	thr->id = 0;                       /* idを初期化                       */

	refcnt_init(&thr->refs);    /* 参照カウンタを初期化(スレッド管理ツリーからの参照分) */
	list_init(&thr->link);      /* スケジューラキューへのリストエントリを初期化         */
	list_init(&thr->proc_link); /* プロセス内のスレッドキューのリストエントリを初期化   */
	list_init(&thr->children_link);    /* 子スレッド一覧へのリンクを初期化              */
	queue_init(&thr->children);        /* 子スレッド一覧を初期化                    */
	queue_init(&thr->waiters);         /* wait待ちスレッドのキューを初期化          */
	wque_init_wait_queue(&thr->pque);  /* wait待ちスレッドの待ちキューを初期化(親)  */

	thr->exitcode = 0;           /* 終了コードを初期化     */

	thr->flags = flags;          /* スレッドの属性値を設定 */

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
	thr->tinfo = calc_thread_info_from_kstack_top(newstk);  /* スレッド情報アドレスを算出  */
	ti_thread_info_init(thr); /* スレッド情報初期化                   */

	thr->ksp = (void *)thr->tinfo;   /* スレッド管理情報を指すようにスタック位置を初期化 */

	/* スレッドスイッチコンテキスト, 例外コンテキストの初期化
	 */
	hal_setup_thread_context(entry, usp, thr->flags, &thr->ksp);

	thr->state = THR_TSTATE_RUNABLE;   /* スレッドを実行可能状態に遷移  */

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

			rc = -ENOSPC;  /* 空IDがない */
			goto unlock_out;
		}
		thr->id = newid - 1;  /* スレッドIDを設定 */
		/* 割当てたIDに対応するビットマップ中のビットを使用中にセット */
		bitops_set(thr->id, &g_thrdb.idmap); 
	}

	res = RB_INSERT(_thrdb_tree, &g_thrdb.head, thr);  /* 生成したスレッドを登録 */

	/* スレッド管理ツリーのロックを解放 */
	spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);
	kassert( res == NULL );

	if ( thrp != NULL )
		*thrp = thr;  /* スレッド情報を返却 */

	return 0;

unlock_out:
	/* スレッド管理ツリーのロックを解放 */
	spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);

	if ( kstktop == NULL )
		pgif_free_page(newstk);  /* 動的に割り当てたスタックを解放 */

free_thr_out:
	slab_kmem_cache_free((void *)thr);  /* スレッド管理情報を解放 */

error_out:
	return rc;
}

/**
   スレッドに子スレッドを追加する (内部関数)
   @param[in] thr    追加する子スレッドのスレッド管理情報
   @param[in] parent 追加先の親スレッド
   @retval    0      正常終了
   @retval   -ENOENT 解放処理中のスレッドを追加しようとした
*/
static int
add_child_thread_nolock(thread *thr, thread *parent){
	int              rc;
	bool            res;

	res = thr_ref_inc(parent);  /* 親スレッドの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;  /* 解放処理中 */
		goto error_out;
	}


	res = thr_ref_inc(thr);  /* 子スレッドの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;  /* 解放処理中 */
		goto unref_parent_out;
	}

	/*
	 * スレッドを親の子スレッドに追加
	 */

	/* 以前の親スレッドキューから取り外し済みであることを確認 */
	kassert( list_not_linked(&thr->children_link) ); 

	/* 親スレッドのキューに追加する */
	queue_add(&parent->children, &thr->children_link);  
	res = thr_ref_inc(thr);   /*  親スレッドのキューから子スレッドの参照を加算    */
	kassert( res );


	thr->parent = parent;  /* 親スレッドを更新する */
	res = thr_ref_inc(thr->parent);  /* 子スレッドから親スレッドへの参照を加算 */
	kassert( res );  /* 親スレッドは常に存在する */

	thr_ref_dec(thr);     /* 子スレッドの参照を解放 */

	thr_ref_dec(parent);  /*  親スレッドの参照を解放 */

	return 0;

unref_parent_out:
	thr_ref_dec(parent);      /*  親スレッドの参照を解放    */

error_out:
	return rc;
}

/**
   指定した子スレッドを取り除く (内部関数)
   @param[in] thr    取り除く子スレッドのスレッド管理情報
   @param[in] parent 削除元の親スレッド
   @note 親スレッドの子スレッドキューから指定したスレッドを取り除くが,
   子スレッドから親スレッドへの通知を行うために親スレッドへの参照は解放できない。
   子スレッドから親スレッドへの参照は, スレッド管理情報への全ての参照が完了し, 
   親プロセスへスレッドの終了を通知可能になった時点(notify_parent_child_exit)で
   解放する。
*/
static void
del_child_thread_nolock(thread *thr, thread *parent){
	bool            res;

	res = thr_ref_inc(parent);  /* 親スレッドの参照を獲得 */
	kassert( res ); /* 解放処理中でないことを確認 */

	res = thr_ref_inc(thr);  /* 子スレッドの参照を獲得 */
	kassert( res ); /* 解放処理中でないことを確認 */

	/*
	 * スレッドを親の子スレッドキューから外す
	 */

	/* スレッドキューに追加されていることを確認 */
	kassert( !list_not_linked(&thr->children_link) ); 
	
	/* 指定された親スレッドの子を削除することを確認 */
	kassert( thr->parent == parent );
	
	/* 親スレッドのキューから削除する */
	queue_del(&parent->children, &thr->children_link);  
	res = thr_ref_dec(thr);   /*  親スレッドのキューから子スレッドの参照を減算    */
	kassert( !res );

	thr_ref_dec(thr);     /* 子スレッドの参照を解放 */

	thr_ref_dec(parent);  /*  親スレッドの参照を解放 */

	return ;
}
/**
   スレッド資源(スレッド管理情報とカーネルスタック)を解放する
   @param[in] thr 
 */
static void
free_thread(thread *thr){

	kassert( refcnt_read(&thr->refs) == 0 );  /* 解放中のスレッドである事を確認             */
	kassert( thr->state == THR_TSTATE_DEAD ); /* 親プロセスへの終了通知済みであることを確認 */

	pgif_free_page(thr->attr.kstack_top);  /* カーネルスタックを解放 */
	slab_kmem_cache_free((void *)thr);     /* スレッド管理情報を解放 */
}

/**
   スレッド資源を回収する (内部関数)
   @param[in] arg 引数へのポインタ
*/
static void
reap_thread(void __unused *arg){
	thr_wait_res child;
	intrflags   iflags;
	
	for( ; ; ) {

		krn_cpu_save_and_disable_interrupt(&iflags);  /* 割込みを禁止する */
		
		thr_thread_wait(&child); /* 子スレッドを待ち合せる */
		
		krn_cpu_restore_interrupt(&iflags);   /* 割込みを許可する */
	}
}

/**
   スレッドの終了を親スレッドに通知する(内部関数)
   @param[in] thr 終了するスレッドのスレッド管理情報
 */
static void
notify_parent_child_exit(thread *thr){
	intrflags    iflags;

	kassert( list_not_linked(&thr->link) );  /* レディキューに繋がっていないことを確認 */

	/**
	   親スレッドからのwaitを待ち合せる
	    @note 親スレッドの参照は生成時に獲得済み
	 */
	spinlock_lock_disable_intr(&thr->parent->lock, &iflags);  /* 親スレッドのロックを獲得 */
	spinlock_lock(&thr->lock);                    /* 終了するスレッドのロックを獲得 */

	queue_add(&thr->parent->waiters, &thr->link);  /* 終了するスレッドを登録    */
	thr->state = THR_TSTATE_DEAD;                 /* 回収待ち中に遷移   */	

	spinlock_unlock(&thr->lock);                    /* 終了するスレッドのロックを解放 */
	spinlock_unlock_restore_intr(&thr->parent->lock, &iflags); /* 親スレッドのロックを解放 */

	wque_wakeup(&thr->parent->pque, WQUE_RELEASED);  /* 親スレッドを起床 */
	thr_ref_dec(thr->parent); /* 子スレッドから親スレッドへの参照を解放         */
}

/**
   スレッドの終了を待ち合わせシステムコール実処理関数
   @param[out] *resp 終了したスレッドの終了時情報格納領域
   @retval  0      正常終了
   @retval -EINTR  イベントを受信した
 */
int
thr_thread_wait(thr_wait_res *resp){
	int              rc;
	bool            res;
	thread         *cur;
	thread         *thr;
	wque_reason  reason;
	intrflags    iflags;

	cur = ti_get_current_thread();  /* 自スレッドの管理情報を取得 */
	res = thr_ref_inc(cur);         /* 自スレッドの参照を取得     */
	if ( !res )
		goto error_out;         /* 終了処理中 */

	/* 自スレッドをロック */
	spinlock_lock_disable_intr(&cur->lock, &iflags);

	if ( queue_is_empty(&cur->waiters) ) {

		/* wait待ちキューで休眠する */
		reason = wque_wait_for_curthr(&cur->pque);
		if ( reason == WQUE_DELIVEV ) {
			
			rc = -EINTR; /* イベントを受信した */
			goto unlock_out;
		}
		kassert( reason == WQUE_RELEASED );	
	}	

	/**
	   wait待ち中の子スレッドを取り出し, 終了状態を取得
	*/
	thr = container_of(queue_get_top(&cur->waiters), thread, link);

	if ( thr->state != THR_TSTATE_DEAD ) {  /*  終了したスレッドでない場合 */

		res = thr_ref_inc(thr);       /*  子スレッドの参照を取得    */
		kassert( !res );
	}

	resp->id = thr->id;             /* 子スレッドのIDを取得     */
	resp->exitcode = thr->exitcode; /* 終了コードを取得         */

	if ( thr->state != THR_TSTATE_DEAD )
		thr_ref_dec(thr);         /*  子スレッドの参照を解放  */

	/* 自スレッドのロックを解放 */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);

	thr_ref_dec(cur);         /*  自スレッドの参照を解放    */

	if ( thr->state == THR_TSTATE_DEAD )
		free_thread(thr);       /*  子スレッドの資源を解放      */

	return 0;

unlock_out:
	/* 自スレッドのロックを解放 */
	spinlock_unlock_restore_intr(&cur->lock, &iflags);

	thr_ref_dec(cur);         /*  自スレッドの参照を解放    */

error_out:
	return rc;
}

/**
   スレッド終了システムコール実処理
   @param[in] ec スレッド終了コード
   @note LO reaperスレッドのロック, reaper内に追加するスレッドのロック
 */
void
thr_thread_exit(exit_code ec){
	bool            res;
	thread          key;
	thread         *cur;
	thread      *reaper;
	thread         *thr;
	list            *lp;
	list            *np;
	intrflags    iflags;

	cur = ti_get_current_thread();  /* 自スレッドの管理情報を取得 */
	res = thr_ref_inc(cur);         /*  自スレッドの参照を取得    */
	if ( !res )
		goto error_out;         /*  終了処理中                */

	kassert( list_not_linked(&cur->link) );  /* レディキューに繋がっていないことを確認 */
	
	cur->exitcode = ec;             /*  終了コードを設定          */

	/* TODO: thr_ref_decで最終参照の場合の処理に移動する - begin - */
	cur->state = THR_TSTATE_EXIT;   /*  終了処理中に遷移          */

	/* 子スレッドをreaper threadの子スレッドに設定する
	 */
	key.id = THR_TID_REAPER;
	/* スレッド管理ツリーのロックを獲得 */
	spinlock_lock_disable_intr(&g_thrdb.lock, &iflags);

	reaper = RB_FIND(_thrdb_tree, &g_thrdb.head, &key);  /* 回収スレッドを検索 */

	/* スレッド管理ツリーのロックを解放 */
	spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);
	kassert( reaper != NULL );

	res = thr_ref_inc(reaper);      /*  回収スレッドの参照を取得    */
	kassert( res );
	
	/**
	   回収スレッドの子スレッドに追加
	 */
	/* 回収スレッドのロックを獲得 */
	spinlock_lock_disable_intr(&reaper->lock, &iflags);

	queue_for_each_safe(lp, &cur->children, np) {

		thr = container_of(lp, thread, children_link);  /* 子スレッドを取り出し     */

		kassert( thr->parent == cur );  /* 自スレッドの子スレッドである事を確認 */
		res = thr_ref_inc(thr);         /*  スレッドの参照を獲得                */
		kassert( res );

		spinlock_lock(&thr->lock);                  /* 子スレッドのロックを獲得 */

		/*
		 * 子スレッドを回収スレッドの子に移動
		 */
		del_child_thread_nolock(thr, cur);    /* スレッドを取り除く */

		res = thr_ref_dec(thr->parent);  /* 子スレッドから親スレッドへの参照を減算 */
		kassert( !res );  /* 上記で参照を得ているので最終参照とはならない */

		thr->parent = NULL;  /* 親スレッドへのリンクを無効化する */

		add_child_thread_nolock(thr, reaper); /* 回収スレッドの子スレッドに追加する */

		spinlock_unlock(&thr->lock);   /* 子スレッドのロックを解放   */
		kassert( thr->parent == reaper);

		res = thr_ref_dec(thr);        /*  子スレッドの参照を解放                       */
		kassert( !res );               /*  親スレッドの子スレッドキューからの参照が残る */
	}

	/* 回収スレッドのロックを解放 */
	spinlock_unlock_restore_intr(&reaper->lock, &iflags);

	res = thr_ref_dec(reaper);      /*  回収スレッドの参照を解放    */
	kassert( !res );

	del_child_thread_nolock(cur, cur->parent);    /* 自スレッドを親から取り除く */
	/* TODO: thr_ref_decで最終参照の場合の処理に移動する - end - */

	thr_ref_dec(cur);         /*  スレッド終了処理用に参照を解放   */

	thr_ref_dec(cur);         /*  自スレッドの参照を解放           */

	sched_schedule();         /*  スレッド終了に伴う再スケジュール */

	return;

error_out:
	return;
}

/**
   スレッド生成処理
   @param[in]  id      スレッドID
   @param[in]  entry   スレッドエントリアドレス
   @param[in]  usp     ユーザスタックポインタ初期値
   @param[in]  kstktop カーネルスタックの先頭アドレス (NULLの場合は動的に割当てる)
   @param[in]  prio    初期化時のスレッド優先度
   @param[in]  flags   スレッド属性フラグ
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
	thread       *thr;
	thread    *parent;
	bool      ref_res;

	if ( !SCHED_VALID_PRIO(prio) )
		return -EINVAL;

	if ( ( flags & THR_THRFLAGS_USER ) && ( !SCHED_VALID_USER_PRIO(prio) ) )
		return -EINVAL;

	/**
	   スレッド管理情報を生成, 登録する
	 */
	rc = create_thread_common(id, entry, usp, kstktop, prio, flags, &thr);
	if ( rc != 0 )
		goto error_out;  /* スレッド生成失敗 */

	parent = ti_get_current_thread(); /* スレッドを生成したスレッドを親スレッドに設定 */
	ref_res = thr_ref_inc(parent);    /* 親スレッドの参照を獲得                       */
	kassert( ref_res );               /* 親スレッドは存在するはず                     */

	add_child_thread_nolock(thr, parent);  /* 親スレッドの子スレッドキューに追加する  */

	ref_res = thr_ref_dec(parent);         /*  親スレッドの参照を解放                   */
	kassert( !ref_res );  /* 自スレッドから親スレッドへの参照があるため最終参照ではない */

	if ( thrp != NULL )
		*thrp = thr;  /* スレッドを返却 */

	return 0;

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
	hal_thread_switch(&prev->ksp, &next->ksp);
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
	thread        *cur;
	intrflags   iflags;

	/*  スレッドの最終参照者であればスレッドを解放する
	 */
	res = refcnt_dec_and_lock_disable_intr(&thr->refs, &g_thrdb.lock, &iflags);
	if ( res ) {  /* 最終参照者だった場合  */

		/* スレッド管理ツリーのロックを解放 */
		spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);

		 /* レディキューに繋がっていないことを確認 */
		kassert( list_not_linked(&thr->link) ); 
		cur = ti_get_current_thread(); /* カレントスレッドを参照 */
		/* スレッド管理情報の解放準備ができたことを親スレッドに通知 */
		notify_parent_child_exit(thr); 
		if ( cur == thr ) 
			sched_schedule();  /*  自コアのカレントスレッドだった場合はCPUを解放 */
	}

	return res;
}
/**
   スレッドIDを返却する
   @param[in] id スレッドID
   @note マスタスレッドが他のスレッドより先に終了したプロセスの
   プロセスIDを返却するために使用
 */
void
release_threadid(tid id){
	intrflags   iflags;
	thread    *thr_res;
	thread        *thr;
	thread         key;

	key.id = id;
	/* スレッド管理ツリーのロックを獲得 */
	spinlock_lock_disable_intr(&g_thrdb.lock, &iflags);

	thr = RB_FIND(_thrdb_tree, &g_thrdb.head, &key);       /* スレッドを検索 */
	kassert( thr != NULL );                                /* スレッドが見つからなかった */

	thr_res = RB_REMOVE(_thrdb_tree, &g_thrdb.head, thr);  /* スレッドをツリーから削除 */
	kassert( thr_res != NULL );

	bitops_clr(id, &g_thrdb.idmap);  /* IDを解放する */

	/* スレッド管理ツリーのロックを解放 */
	spinlock_unlock_restore_intr(&g_thrdb.lock, &iflags);
}

/**
   アイドルスレッドループ関数
   @param[in] arg 引数へのポインタ
*/
void
thr_idle_loop(void __unused *arg){
	intrflags iflags;

	krn_cpu_enable_interrupt();                   /* 割込みを許可する */

	for( ; ; ) {

		krn_cpu_save_and_disable_interrupt(&iflags);  /* 割込みを禁止する */

		if ( ti_dispatch_delayed() ) 
			sched_schedule();  /* ディスパッチ要求に従って再スケジュール */
		else {

			/** 
			    @note 多くのCPUでは, 割込み禁止状態に遷移した後でCPU休眠命令を
			    発行することでCPUの休眠と割込み通知とのレースコンディションを
			    避ける機能をCPU休眠命令が提供しているのでアーキごとに休眠処理を
			    実装する
			 */
			hal_cpu_halt(); /* 割込み待ちでプロセッサを休眠させる */
		}
		krn_cpu_restore_interrupt(&iflags);   /* 割込みを許可する */
	}
}

/**
   アイドルスレッドを生成する
   @param[in] cpu  論理プロセッサ番号
   @param[in] thrp スレッド管理情報を指し示すポインタの格納先アドレス
   @retval    0    正常終了
   @retval    -EINVAL 不正な優先度を指定した
   @retval    -ENOMEM メモリ不足
   @retval    -ENOSPC スレッドIDに空きがない
 */
int
thr_idlethread_create(cpu_id cpu, thread **thrp){
	int          rc;
	thread_info *ti;
	thread     *thr;
	tid          id;

	ti = ti_get_current_thread_info(); /* スタック上のスレッド情報を参照 */
	/** アイドルスレッドを生成
	 */
	if ( cpu == 0 )
		id = THR_TID_IDLE;  /* 論理プロセッサ番号0のアイドルスレッドに0番を割振る */
	else
		id = THR_TID_AUTO;  /* アイドルスレッドの番号を自動的に割振る */

	rc = create_thread_common(id, (vm_vaddr)thr_idle_loop, NULL, ti->kstack,
	    SCHED_MIN_RR_PRIO, THR_THRFLAGS_KERNEL, &thr);
	if ( rc != 0 )
		goto error_out;

	thr->parent = thr;  /* 自分自身を参照 */

	if ( thrp != NULL )
		*thrp = thr;   /* スレッド管理情報を返却 */

	return 0;

error_out:
	return rc;
}

/**
   システムスレッドを生成する
 */
void
thr_system_thread_create(void){
	int          rc;
	thread     *thr;

	/**
	   回収スレッドを生成
	 */
	rc = create_thread_common(THR_TID_REAPER, (vm_vaddr)reap_thread, NULL, NULL,
				  THR_PRIO_REAPER, THR_THRFLAGS_KERNEL, &thr);
	kassert( rc == 0 );

	thr->parent = thr;  /* 自分自身を参照 */

	sched_thread_add(thr);  /* 回収スレッドを実行可能にする */

	return ;
}

/**
   スレッド情報管理機構を初期化する
 */
void
thr_init(void){
	int          rc;
	int           i;

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
