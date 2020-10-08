/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  OS kernel sample                                                  */
/*  Copyright 2019 Takeharu KATO                                      */
/*                                                                    */
/*  Page Cache                                                        */
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

static kmem_cache pcache_cache;  /* ページキャッシュのSLABキャッシュ */
/** ページキャッシュプール
 */
static page_cache_pool pcache_pool = __PCACHE_POOL_INITIALIZER(&pcache_pool, PAGE_SIZE);

/** ページキャッシュプールSPLAY木
 */
static int _page_cache_ent_cmp(struct _page_cache *_key, struct _page_cache *_ent);
SPLAY_GENERATE_STATIC(_pcache_tree, _page_cache, ent, _page_cache_ent_cmp);

/**
    ページキャッシュプールエントリ比較関数 (内部関数)
    @param[in] key 比較対象領域1
    @param[in] ent SPLAY木内の各エントリ
    @retval 正  keyが entより前にある
    @retval 負  keyが entより後にある
    @retval 0   keyが entに等しい
 */
static int
_page_cache_ent_cmp(struct _page_cache *key, struct _page_cache *ent){

	if ( key->bdevid < ent->bdevid )
		return 1;

	if ( key->bdevid > ent->bdevid )
		return -1;

	if ( key->offset < ent->offset )
		return 1;

	if ( key->offset > ent->offset )
		return -1;

	return 0;
}

/**
   ページキャッシュ管理情報とページキャッシュに割り当てたページを解放する (内部関数)
   @param[in]      pc      ページキャッシュ管理情報
 */
static void
free_pagecache_nolock(page_cache *pc){

       kassert( PCACHE_IS_BUSY(pc) );   /* ページの更新権を得ていることを確認 */
       kassert( !PCACHE_IS_DIRTY(pc) ); /* 2次記憶へのキャッシュ書き出し完了を確認 */
       kassert( !mutex_locked_by_self(&pc->mtx) ); /* ページmutex解放済みであることを確認 */
       /* ページキャッシュプールのロックを獲得済みであることを確認 */
       kassert( spinlock_locked_by_self(&pcache_pool.lock) );

       pc->state &= ~PCACHE_STATE_BUSY;  /* ページのロックを解放する */

       pgif_free_page(pc->pc_data);      /* ページキャッシュページの解放   */
       slab_kmem_cache_free((void *)pc); /* ページキャッシュ管理情報の解放 */
}

/**
   ページキャッシュの割り当て (内部関数)
   @param[out] pcp ページキャッシュ管理情報アドレス返却域
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
 */
static int
allocate_new_pcache(page_cache **pcp){
	int               rc;
	page_cache       *pc;
	page_frame       *pf;
	void        *newpage;

	/* ページキャッシュ用にページを獲得  */
	rc = pgif_get_free_page(&newpage, KMALLOC_NORMAL, PAGE_USAGE_PCACHE);
	if ( rc != 0 )
		return -ENOMEM;   /* メモリ不足  */

	rc = pfdb_kvaddr_to_page_frame(newpage, &pf);
	kassert( rc == 0 ); /* ページプール内のページであるため必ず成功する */

	rc = slab_kmem_cache_alloc(&pcache_cache, KMALLOC_NORMAL, (void **)&pc);
	if ( rc != 0 ) {

		kprintf("Cannot allocate page cache\n");
		goto free_newpage_out;
	}

	mutex_init(&pc->mtx);               /* ページmutexの初期化          */
	wque_init_wait_queue(&pc->waiters); /* ページウエイトキューの初期化 */
	/* 参照カウンタの初期化 (ページキャッシュプールからの参照分として1で初期化)  */
	refcnt_init(&pc->refs);
	pc->bdevid  = 0;                     /* 無効デバイスIDに設定               */
	pc->offset  = 0;                     /* オフセットアドレスを初期化         */
	pc->pgsiz   = 0;                     /* ページサイズを初期化               */
	pc->state   = PCACHE_STATE_INVALID;  /* 無効キャッシュに設定               */
	pc->pf      = pf;                    /* ページフレーム情報を設定           */
	pc->pc_data = newpage;               /* ページアドレスを初期化             */
	pc->pf->pcachep = pc;                /* ページキャッシュへの逆リンクを設定 */
	*pcp = pc;  /* ページキャッシュ管理情報を返却 */

	return 0;

free_newpage_out:
	/* ページキャッシュページ解放 */
	pgif_free_page(newpage);
	return rc;
}

/**
   ページキャッシュに2次記憶の内容を読み込む (内部関数)
   @param[in]  pc     ページキャッシュ
 */
static void
fill_pagecache(page_cache *pc){

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */
	kassert( !PCACHE_IS_VALID(pc) ); /* 有効なキャッシュでない */

	mutex_unlock(&pc->mtx);  /* ページmutexを解放 */

	/* TODO: ブロックデバイスからの読込み */
	fsimg_strategy(pc);

	mutex_lock(&pc->mtx);  /* ページmutexを獲得 */

	/* ページキャッシュと2次記憶との同期が完了しているので
	 * PCACHE_STATE_CLEAN (2次記憶との同期が完了している)を設定する
	 */
	pc->state |= PCACHE_STATE_CLEAN;
}

/**
   ページキャッシュの獲得を待ち合わせる  (内部関数)
   @param[in]  pc     ページキャッシュ管理情報
   @retval     0      正常終了
   @retval    -ENOENT キャッシュの解放待ち中にページを破棄した
   @retval    -EINTR  キャッシュの解放待ち中にイベントを受け付けた
 */
static int
wait_on_pagecache_nolock(page_cache *pc){
	int               rc;
	wque_reason   reason;

	/* ページキャッシュプールのロックを保持した状態で呼びしていることを確認 */
	kassert( spinlock_locked_by_self(&pcache_pool.lock) );

	/* 他のスレッドがキャッシュを使用中だった場合は,
	 * キャッシュの解放を待ち合わせる
	 */
	while( PCACHE_IS_BUSY(pc) ) {

		/* ページのウエイトキューで休眠する
		 */
		reason = wque_wait_on_queue_with_spinlock(&pc->waiters,
		    &pcache_pool.lock);

		if ( reason == WQUE_DESTROYED ) {  /* ページが破棄された */

			rc = -ENOENT; /* ページキャッシュが見つからなかった */
			goto error_out;
		}

		if ( reason == WQUE_DELIVEV ) {

			rc = -EINTR; /* イベントを受信した */
			goto error_out;
		}
		kassert( reason == WQUE_RELEASED );
	}

	return 0;

error_out:
	return rc;
}
/**
   ページキャッシュプールからページキャッシュを検索する (内部関数)
   @param[in]  dev    ブロックデバイスID
   @param[in]  offset オフセットページアドレス
   @param[out] pcp    ページキャッシュ管理情報返却領域
   @retval     0      正常終了
   @retval    -ENOENT キャッシュ中に指定されたページがない
   @retval    -EINTR  キャッシュの解放待ち中にイベントを受け付けた
 */
static int
lookup_pagecache(dev_id dev, off_t offset, page_cache **pcp){
	int               rc;
	page_cache       key;
	page_cache       *pc;
	page_cache      *res;
	page_cache      *new;

	/* ページキャッシュプールのロックを獲得 */
	spinlock_lock(&pcache_pool.lock);

	key.bdevid = dev;  /* 検索対象デバイス */
	/* オフセットをページキャッシュ境界に合わせる */
	key.offset = truncate_align(offset, pcache_pool.pgsiz);

	/*
	 * ページキャッシュプールから指定されたデバイス上のページキャッシュを検索する
	 */
	for( ; ; ) {

		pc = SPLAY_FIND(_pcache_tree, &pcache_pool.head, &key);
		if ( pc != NULL ) { /* キャッシュから見つかった場合 */

			/* 他のスレッドがキャッシュを使用中だった場合は,
			 * キャッシュの解放を待ち合わせる
			 */
			rc = wait_on_pagecache_nolock(pc);
			if ( rc != 0 )
				goto unlock_out; /* イベント受信かページが破棄された */

			/* ページキャッシュのロックが解放されていることを確認する */
			kassert( !PCACHE_IS_BUSY(pc) );
			pc->state |= PCACHE_STATE_BUSY;  /* ページをロックする  */

			/* LRUにつながっていることを確認したらLRUから外す
			 *
			 * @note シュリンカがページを処理した際に
			 * 対象ページを待ち合わせている場合は, LRUから
			 * 外して, ページ獲得～ページ読み書き処理後の
			 * ロック解放時にLRUへの登録を期待する。
			 * このため, 上記条件の場合LRUから外れたページを
			 * 獲得することになるためLRUへの接続有無を判定する必要がある。
			 */
			if (!list_not_linked(&pc->pf->lru_ent)) {

				/* LRU参照分の参照カウンタを減算 */
				kassert(!refcnt_dec_and_test(&pc->refs));
				/* LRUから削除 */
				queue_del(&pcache_pool.lru, &pc->pf->lru_ent);
				break;   /* ページキャッシュ獲得完了 */
			}
		} else {
			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock(&pcache_pool.lock);

			/* ページキャッシュプール内にページがなかった場合,
			 * ページキャッシュを割り当て, BUSY状態に遷移しキャッシュに登録
			 */

			/* ページキャッシュ情報を割り当て  */
			rc = allocate_new_pcache(&new);
			if ( rc != 0 ) {

				rc = -ENOENT;  /* ページキャッシュ獲得失敗 */
				goto error_out;
			}

			new->bdevid  = dev;          /* 無効デバイスIDに設定         */
			new->offset  = key.offset;   /* オフセットアドレスを設定     */
			new->pgsiz   = pcache_pool.pgsiz; /* ページサイズを設定      */
			/* ページ未読み込みに設定しページをロック  */
			new->state = (PCACHE_STATE_BUSY|PCACHE_STATE_INVALID);

			/* ページキャッシュプールのロックを獲得 */
			spinlock_lock(&pcache_pool.lock);

			/* ページキャッシュを登録 */
			res = SPLAY_INSERT(_pcache_tree, &pcache_pool.head, new);
			if ( res == NULL ) { /* 登録成功 */

				pc = new; /* 新規に割り当てページキャッシュを使用 */
				break;    /* ページキャッシュ登録完了             */
			}

			/*
			 * 登録済みのキャッシュを再検索するため割り当てたメモリを解放
			 */
			/* ページキャッシュの解放  */
			free_pagecache_nolock(new);
		}
	}

	/* 参照カウンタをインクリメント
	 * ページキャッシュプールからの参照があるので必ず成功する
	 */
	kassert(refcnt_inc_if_valid(&pc->refs));

	/* LRUに繋がっていないことを確認 */
	kassert(list_not_linked(&pc->pf->lru_ent));

	/* ページキャッシュプールのロックを解放 */
	spinlock_unlock(&pcache_pool.lock);

	kassert( PCACHE_IS_BUSY(pc) ); /* ページキャッシュロック済みであることを確認する */

	mutex_lock(&pc->mtx);  /* ページmutexを獲得する */
	if ( !PCACHE_IS_VALID(pc) ) {

		/* 無効なページキャッシュだった場合は,
		 * ページの内容をロードする
		 */
		fill_pagecache(pc);
	}

	*pcp = pc;  /* ページキャッシュを返却する */

	return 0;

unlock_out:
	/* ページキャッシュプールのロックを解放 */
	spinlock_unlock(&pcache_pool.lock);
error_out:
	return rc;
}

/**
   ページキャッシュの参照を落とし最終参照者だった場合にページキャッシュを解放する (内部関数)
   @param[in]  pc     ページキャッシュ
   @retval     真     ページキャッシュの最終参照者だった
   @retval     偽     ページキャッシュの最終参照者でなかった
 */
static bool
dec_pcache_reference(page_cache *pc){
	bool              rc;
	page_cache      *res;

	/* ページキャッシュmutexを保持していないことを確認 */
	kassert( !mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */

	/* ページの参照を落とす
	 * 最終参照者だった場合ページキャッシュプールからページを取り除くために
	 * ページキャッシュプールのロックを獲得する
	 */
	rc = refcnt_dec_and_lock(&pc->refs, &pcache_pool.lock);
	if ( rc ) {  /* ページの最終参照者だった場合 */

		/* 解放対象のページで待っているスレッドがいる場合は,
		 * ページキャッシュの登録抹消を取りやめる
		 */
		if ( !wque_is_empty(&pc->waiters) ) { /* 解放対象のページ待ちスレッドがいる */

			/* ページキャッシュプールからの参照分を再設定  */
			refcnt_init(&pc->refs);
			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock(&pcache_pool.lock);
			goto false_out;
		}

		if ( PCACHE_IS_DIRTY(pc) ) { /* キャッシュの方が2時記憶より新しい場合 */

			/* ページキャッシュの方が2時記憶より新しい場合は,
			 * ページの書き出しを行いページキャッシュプールから
			 * ページの登録を抹消可能な状態にしてからページを解放する
			 *
			 * ページをロックした状態(PCACHE_IS_BUSY(pc))であることから
			 * この間に他のスレッドがページの内容を更新することはない
			 */

			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock(&pcache_pool.lock);

			/* ページの内容を更新するためにページmutexを獲得する
			 */
			mutex_lock(&pc->mtx);

			pagecache_write(pc);     /* ページを書き戻す  */
			/* これ以降ページの内容を更新する処理はないので,
			 * ページmutexを解放する
			 */
			mutex_unlock(&pc->mtx);

			/* ページキャッシュプールのロックを獲得 */
			spinlock_lock(&pcache_pool.lock);
		}
		kassert( PCACHE_IS_CLEAN(pc) );  /* ページ書き出し済みであることを確認 */

		/* ページキャッシュプールからページの登録を抹消 */
		res = SPLAY_REMOVE(_pcache_tree, &pcache_pool.head, pc);
		kassert( res != NULL );

		free_pagecache_nolock(pc); /* ページキャッシュの解放  */

		/* ページキャッシュプールのロックを解放 */
		spinlock_unlock(&pcache_pool.lock);
	}

	return rc;

false_out:
	return false;
}

/**
   ページキャッシュのページサイズを取得する
   @param[in]  dev    ブロックデバイスID
   @param[out] pgsizp ページサイズ返却領域
   @retval     0      正常終了
 */
int
pagecache_pagesize(dev_id __unused dev, size_t *pgsizp){

	kassert( pgsizp != NULL );

	/* ページキャッシュプールのロックを獲得 */
	spinlock_lock(&pcache_pool.lock);

	*pgsizp = pcache_pool.pgsiz;  /* ページサイズを返却 */

	/* ページキャッシュプールのロックを解放 */
	spinlock_unlock(&pcache_pool.lock);

	return 0;
}

/**
   ページキャッシュを獲得する
   @param[in]  dev    ブロックデバイスID
   @param[in]  offset オフセットページアドレス
   @param[out] pcp    ページキャッシュ返却領域
   @retval     0      正常終了
   @retval    -ENOENT キャッシュ中に指定されたページがない
   @retval    -EINTR  キャッシュの解放待ち中にイベントを受け付けた
 */
int
pagecache_get(dev_id dev, off_t offset, page_cache **pcp){
	int               rc;
	page_cache       *pc;

	rc = lookup_pagecache(dev, offset, &pc);  /* ページキャッシュを検索する */
	if ( rc != 0 )
		return rc;  /* イベント受信 */

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) );  /* 自スレッドがページをロックしている */
	kassert( PCACHE_IS_VALID(pc) ); /* キャッシュが有効である */
	kassert( pc->bdevid == dev);

	*pcp = pc;  /* ページキャッシュを返却する */

	return 0;
}
/**
   ページキャッシュに読み込む
   @param[in]  dev    ブロックデバイスID
   @param[in]  offset オフセットページアドレス
   @param[out] pcp    ページキャッシュ返却領域
   @retval     0      正常終了
   @retval    -ENOENT キャッシュ中に指定されたページがない
   @retval    -EINTR  キャッシュの解放待ち中にイベントを受け付けた
   @note      pagecache_getの別名として実装
 */
int
pagecache_read(dev_id dev, off_t offset, page_cache **pcp){

	/* ページキャッシュを検索, キャッシュされてなければ
	 * キャッシュを新設して, 2次記憶の内容を読み込む
	 */
	return pagecache_get(dev, offset, pcp);
}

/**
   ページキャッシュを解放する
   @param[in]  pc     ページキャッシュ
 */
void
pagecache_put(page_cache *pc){

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */

	/* ページのロックを解放するため, ページmutexを解放して,
	 * ページキャッシュプールのロックを獲得する
	 */
	mutex_unlock(&pc->mtx);  /* ページmutexを解放する */

	/* ページの参照を落とす
	 * 最終参照者だった場合は, ページキャッシュを解放してぬける
	 */
	if ( dec_pcache_reference(pc) )
		goto free_out;  /* 最終参照者だったためページを解放した */


	/* ページキャッシュプールのロックを獲得 */
	spinlock_lock(&pcache_pool.lock);
	kassert(refcnt_inc_if_valid(&pc->refs));        /* LRU参照分の参照カウンタを加算 */
	queue_add(&pcache_pool.lru, &pc->pf->lru_ent);  /* LRUの末尾に追加  */

	pc->state &= ~PCACHE_STATE_BUSY;  /* ページのロックを解放する */
	wque_wakeup(&pc->waiters, WQUE_RELEASED);  /* ページ待ちスレッドを起床する */

	/* ページキャッシュプールのロックを解放 */
	spinlock_unlock(&pcache_pool.lock);

free_out:
	return;
}

/**
   ページキャッシュをダーティ状態に遷移する
   @param[in]  pc     ページキャッシュ
 */
void
pagecache_mark_dirty(page_cache *pc){

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */

	/* ページキャッシュのほうが2次記憶より新しい状態となるので
	 * PCACHE_STATE_CLEAN (2次記憶との同期が完了している)を落とし,
	 * PCACHE_STATE_DIRTY (ページキャッシュのほうが2次記憶より新しい)
	 * を設定する
	 */
	pc->state &= ~PCACHE_STATE_CLEAN;
	pc->state |= PCACHE_STATE_DIRTY;
}

/**
   ページキャッシュを2次記憶に書き戻す
   @param[in]  pc     ページキャッシュ
 */
void
pagecache_write(page_cache *pc){

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */

	mutex_unlock(&pc->mtx);  /* ページmutexを解放 */

	/* TODO: ブロックデバイスへの書き戻し */
	fsimg_strategy(pc);

	mutex_lock(&pc->mtx);  /* ページmutexを獲得 */

	/* ページキャッシュと2次記憶との同期が完了しているので
	 * PCACHE_STATE_DIRTY (ページキャッシュのほうが2次記憶より新しい)を落とし,
 	 * PCACHE_STATE_CLEAN (2次記憶との同期が完了している)を設定する
	 */
	pc->state &= ~PCACHE_STATE_DIRTY;
	pc->state |= PCACHE_STATE_CLEAN;
}
/**
   LRUの内容に従ってページの2次記憶への書き出し、解放を実施する
   @param[in]  max       最大解放ページ数(単位: ページ)
   @param[in]  wbonly    ページを解放せず２次記憶への書き出しのみを行う
   @param[out] free_nrp  解放したページ数を返却する領域
 */
void
pagecache_shrink_pages(obj_cnt_type max, bool wbonly, obj_cnt_type *free_nrp){
	obj_cnt_type free_nr;
	list             *lp;
	list           *next;
	page_cache       *pc;
	page_frame       *pf;

	free_nr = 0;

	/* ページキャッシュプールのロックを獲得 */
	spinlock_lock(&pcache_pool.lock);
	queue_for_each_safe(lp, &pcache_pool.lru, next) {

		/* キューの先頭要素を取り出し */
		pf = container_of(queue_get_top(&pcache_pool.lru), page_frame, lru_ent);
		kassert( pf->pcachep != NULL );

		pc = pf->pcachep;  /* ページキャッシュアドレスを獲得 */
		/* ロックされているページがLRUに繋がっていないことを確認
		 * LRU中のページに対する操作は,
		 * 1) DIRTYなページを2次記憶に書き戻してCLEANに遷移する
		 * 2) CLEANなページを解放する
		 * の2つであるので, いずれのケースにおいてもBUSY(使用中)ページに対しては
		 * 行えない操作である。したがって, pagecache_getがBUSYビットをセットした
		 * 時点でLRUから外していなければならない
		 */
		kassert( !PCACHE_IS_BUSY(pc) );

		pc->state |= PCACHE_STATE_BUSY;  /* ページをロックする  */

		/* ページ操作用に参照カウンタをインクリメント
		 * ページキャッシュプールからの参照があるので必ず成功する
		 */
		kassert(refcnt_inc_if_valid(&pc->refs));

		/* LRU参照分の参照カウンタを減算
		 */
		kassert(!refcnt_dec_and_test(&pc->refs));
		queue_del(&pcache_pool.lru, &pc->pf->lru_ent);  /* LRUから削除  */

		/* ページキャッシュプールのロックを解放 */
		spinlock_unlock(&pcache_pool.lock);

		mutex_lock(&pc->mtx);  /* ページmutexを獲得する */

		if ( PCACHE_IS_DIRTY(pc) )
			pagecache_write(pc);     /* ページを書き戻す  */

		mutex_unlock(&pc->mtx);  /* ページmutexを解放する */

		/*
		 * ページの書き出しのみを行う場合は, 次のページに処理を移行する
		 */
		if ( wbonly ) {

			/* ページキャッシュプールのロックを獲得 */
			spinlock_lock(&pcache_pool.lock);
			continue;
		}

		kassert(!refcnt_dec_and_test(&pc->refs));  /*ページ操作用の参照を解放 */
		kassert( PCACHE_IS_CLEAN(pc) ); /* ページ書き出し済みであることを確認 */

		/* 対象のページを待ち合わせているスレッドがいた場合解放を取りやめる
		 * 対象のページは, 起床されたスレッドのページ操作後に, LRUの
		 * 末尾に追加されるので本処理ではLRUへの追加は不要。
		 * このようにすることでLRU操作箇所を局所化し, 排他処理の単純化を
		 * はかる。
		 */
		/* ページキャッシュプールのロックを獲得 */
		spinlock_lock(&pcache_pool.lock);
		if ( !wque_is_empty(&pc->waiters) ) {

			pc->state &= ~PCACHE_STATE_BUSY;  /* ページのロックを解放する */

			/* ページ待ちスレッドを起床する */
			wque_wakeup(&pc->waiters, WQUE_RELEASED);

			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock(&pcache_pool.lock);
			continue;
		}

		/* ページキャッシュプールのロックを解放 */
		spinlock_unlock(&pcache_pool.lock);

		/* ページの解放を試みる
		 * 他スレッドから参照されおらず(自スレッドがページをロックしており), かつ,
		 * 自身のページ操作用参照を落としており, かつ,
		 * LRUから削除済みでであることからページの解放を試みる
		 */
		if ( dec_pcache_reference(pc) ) {

			++free_nr;  /* 解放ページ数を加算 */
			if ( free_nr == max )
				break;  /* 最大解放ページ数に達した */
		}
		/* ページキャッシュプールのロックを獲得 */
		spinlock_lock(&pcache_pool.lock);
	}

	*free_nrp = free_nr;

	return ;
}
/**
   ページキャッシュ機構の初期化
 */
void
pagecache_init(void){
	int             rc;

	/*
	 * ページキャッシュのSLABキャッシュを初期化する
	 */
	rc = slab_kmem_cache_create(&pcache_cache, "page-cache cache", sizeof(page_cache),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}
