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
	pc->bdevid  = 0;                     /* 無効デバイスIDに設定         */
	pc->offset  = 0;                     /* オフセットアドレスを初期化   */
	pc->state   = PCACHE_STATE_INVALID;  /* 無効キャッシュに設定         */
	pc->pf      = pf;                    /* ページフレーム情報を設定     */
	pc->pc_data = newpage;               /* ページアドレスを初期化       */

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
	intrflags     iflags;	

	/* ページキャッシュプールのロックを獲得 */
	spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);

	key.bdevid = dev;  /* 検索対象デバイス */
	/* オフセットをページキャッシュ境界に合わせる */
	key.offset = truncate_align(offset, pcache_pool.pgsiz);

	/*
	 * ページキャッシュプールから指定されたデバイス上のページキャッシュを検索する
	 */
	for( ; ; ) {

		pc = SPLAY_FIND(_pcache_tree, &pcache_pool.head, &key);
		if ( pc != NULL ) {
	
			/* 他のスレッドがキャッシュを使用中だった場合は, 
			 * キャッシュの解放を待ち合わせる
			 */
			rc = wait_on_pagecache_nolock(pc);
			if ( rc != 0 )
				goto unlock_out; /* イベント受信かページが破棄された */

			/* ページキャッシュのロックが解放されていることを確認する */
			kassert( !PCACHE_IS_BUSY(pc) ); 
			pc->state |= PCACHE_STATE_BUSY;  /* ページをロックする  */
			break;   /* ページキャッシュ獲得完了 */
		} else {
			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
			
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
			new->offset  = key.offset;   /* オフセットアドレスを初期化   */
			
			/* ページ未読み込みに設定しページをロック  */
			new->state = (PCACHE_STATE_BUSY|PCACHE_STATE_INVALID);

			/* ページキャッシュプールのロックを獲得 */
			spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);

			/* ページキャッシュを登録 */
			res = SPLAY_INSERT(_pcache_tree, &pcache_pool.head, new);
			if ( res == NULL ) { /* 登録成功 */

				pc = new; /* 新規に割り当てページキャッシュを使用 */
				break;  /* ページキャッシュ登録完了 */
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
	/* ページキャッシュプールのロックを解放 */
	spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);

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
	spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
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
	intrflags     iflags;

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */

	krn_cpu_save_and_disable_interrupt(&iflags); /* 割込みを禁止する */

	/* ページの参照を落とす
	 * 最終参照者だった場合ページキャッシュプールからページを取り除くために
	 * ページキャッシュプールのロックを獲得する
	 */
	rc = refcnt_dec_and_lock(&pc->refs, &pcache_pool.lock);
	if ( rc ) {  /* ページの最終参照者だった場合 */

		if ( PCACHE_IS_DIRTY(pc) ) { /* キャッシュの方が2時記憶より新しい場合 */
			
			/* ページキャッシュの方が2時記憶より新しい場合は,
			 * ページの書き出しを行いページキャッシュプールから
			 * ページの登録を抹消可能な状態にしてからページを解放する
			 *
			 * ページをロックした状態(PCACHE_IS_BUSY(pc))であることから
			 * この間に他のスレッドがページの内容を更新することはない
			 */

			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);

			pagecache_write(pc);     /* ページを書き戻す  */

			/* ページキャッシュプールのロックを獲得 */
			spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);
		}
		kassert( PCACHE_IS_CLEAN(pc) );  /* ページ書き出し済みであることを確認 */
		
		/* 解放対象のページで待っているスレッドがいる場合は, 
		 * ページキャッシュの登録抹消を取りやめる
		 */
		if ( !wque_is_empty(&pc->waiters) ) { /* 解放対象のページ待ちスレッドがいる */

			/* ページキャッシュプールからの参照分を再設定  */
			refcnt_init(&pc->refs);          
			/* ページキャッシュプールのロックを解放 */
			spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
			goto free_out;
		}

		/* これ以降ページの内容を更新する処理はないので, 
		 * ページmutexを解放する
		 */
		mutex_unlock(&pc->mtx);

		/* ページキャッシュプールからページの登録を抹消 */
		res = SPLAY_REMOVE(_pcache_tree, &pcache_pool.head, pc);
		kassert( res != NULL );

		free_pagecache_nolock(pc); /* ページキャッシュの解放  */

		/* ページキャッシュプールのロックを解放 */
		spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
		goto free_out;
	}
	krn_cpu_restore_interrupt(&iflags);  /* 割込みを復元する */

free_out:
	return rc;
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
   ページキャッシュを解放する
   @param[in]  pc     ページキャッシュ
 */
void
pagecache_put(page_cache *pc){
	intrflags     iflags;

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( PCACHE_IS_BUSY(pc) ); /* 自スレッドがページをロックしている */

	/* ページの参照を落とす
	 * 最終参照者だった場合は, ページキャッシュを解放してぬける
	 */
	if ( dec_pcache_reference(pc) )
		goto free_out;  /* 最終参照者だったためページを解放した */

	/* ページのロックを解放するため, ページmutexを解放して,
	 * ページキャッシュプールのロックを獲得する
	 */
	mutex_unlock(&pc->mtx);  /* ページmutexを解放する */

	/* ページキャッシュプールのロックを獲得 */
	spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);

	pc->state &= ~PCACHE_STATE_BUSY;  /* ページのロックを解放する */
	wque_wakeup(&pc->waiters, WQUE_RELEASED);  /* ページ待ちスレッドを起床する */

	/* ページキャッシュプールのロックを解放 */
	spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);

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

	mutex_lock(&pc->mtx);  /* ページmutexを獲得 */

	/* ページキャッシュと2次記憶との同期が完了しているので
	 * PCACHE_STATE_DIRTY (ページキャッシュのほうが2次記憶より新しい)を落とし,
 	 * PCACHE_STATE_CLEAN (2次記憶との同期が完了している)を設定する
	 */
	pc->state &= ~PCACHE_STATE_DIRTY;
	pc->state |= PCACHE_STATE_CLEAN;
}

/**
   ページキャッシュ機構の初期化
 */
void
pagecache_init(void){
	int             rc;
	off_t          off;
	page_cache     *pc;
	page_cache    *res;
	intrflags   iflags;

	/*
	 * ページキャッシュのSLABキャッシュを初期化する
	 */
	rc = slab_kmem_cache_create(&pcache_cache, "page-cache cache", sizeof(page_cache),
	    SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
	
	/*
	 * ファイルシステムイメージ(FSIMG)のページキャッシュを登録
	 * TODO: ファイルシステムイメージ(FSIMG)処理部に移動
	 */
	for(off = 0;
	    (off_t)((uintptr_t)&_fsimg_end - (uintptr_t)&_fsimg_start) > off;
	    off += PAGE_SIZE) {

		rc = allocate_new_pcache(&pc);  /* ページキャッシュ管理情報の割り当て */
		kassert( rc == 0 );

		pc->bdevid = FS_FSIMG_DEVID;     /* FSIMGデバイスに設定        */
		pc->offset = off;                /* オフセットアドレスを設定   */
		pc->state = PCACHE_STATE_CLEAN;  /* キャッシュ読込み済みに設定 */
		/* ページをコピー */
		memcpy(pc->pc_data, (void *)((uintptr_t)&_fsimg_start + off), PAGE_SIZE);
		
		/*
		 * ページキャッシュプールに登録
		 */

		/* ページキャッシュプールのロックを獲得 */
		spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);

		res = SPLAY_INSERT(_pcache_tree, &pcache_pool.head, pc);

		/* ページキャッシュプールのロックを解放 */
		spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
		kassert(res == NULL);
	}
}
