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
static page_cache_pool __unused pcache_pool = __PCACHE_POOL_INITIALIZER(&pcache_pool.head, PAGE_SIZE);

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
   ページキャッシュ管理情報の割り当て (内部関数)
   @param[out] pcp ページキャッシュ管理情報アドレス返却域
   @retval  0      正常終了
   @retval -ENOMEM メモリ不足
 */
static int
allocate_new_pcache(page_cache **pcp){
	int         rc;
	page_cache *pc;

	rc = slab_kmem_cache_alloc(&pcache_cache, KMALLOC_NORMAL, (void **)&pc);
	if ( rc != 0 ) {
		
		kprintf("Cannot allocate page cache\n");
		return rc;
	}

	mutex_init(&pc->mtx);               /* ページmutexの初期化          */
	wque_init_wait_queue(&pc->waiters); /* ページウエイトキューの初期化 */
	pc->bdevid = 0;                     /* 無効デバイスIDに設定         */
	pc->offset = 0;                     /* オフセットアドレスを初期化   */
	pc->state = PCACHE_STATE_INVALID;   /* 無効キャッシュに設定         */
	pc->pc_data = NULL;                 /* ページアドレスを初期化       */

	*pcp = pc;  /* ページキャッシュ管理情報を返却 */

	return 0;
}

/**
   ページキャッシュプールからページキャッシュを検索する (内部関数)
   @param[in]  dev    ブロックデバイスID
   @param[in]  offset オフセットページアドレス
   @param[out] pcp    ページキャッシュ返却領域
   @retval     0      正常終了
   @retval    -ENOENT キャッシュ中に指定されたページがない
   @retval    -EINTR  キャッシュの解放待ち中にイベントを受け付けた
 */
static int
lookup_pagecache(dev_id dev, off_t offset, page_cache **pcp){
	int               rc;
	page_cache       key;
	page_cache      *res;
	wque_reason   reason;
	intrflags     iflags;	

	spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);

	key.bdevid = dev;  /* 検索対象デバイス */
	/* オフセットをページキャッシュ境界に合わせる */
	key.offset = truncate_align(offset, pcache_pool.pgsiz);

	/*
	 * ページキャッシュプールから指定されたデバイス上のページキャッシュを検索する
	 */
	res = SPLAY_FIND(_pcache_tree, &pcache_pool.head, &key);
	if ( res == NULL ) {

		rc = -ENOENT;  /* ページキャッシュが見つからなかった */
		goto unlock_out;
	}

	/* 他のスレッドがキャッシュを使用中だった場合は, 
	 * キャッシュの解放を待ち合わせる
	 */
	while( ( res->state & PCACHE_STATE_BUSY ) != 0 ) {
		
		/* ページのウエイトキューで休眠する
		 */
		reason = wque_wait_on_queue_with_spinlock(&res->waiters, 
		    &pcache_pool.lock);
		if ( reason == WQUE_DESTROYED ) {  /* ページが破棄された場合 */
			
			rc = -ENOENT; /* ページキャッシュが見つからなかった */
			goto unlock_out;
		}
		if ( reason == WQUE_DELIVEV ) {

			rc = -EINTR; /* イベントを受信した */
			goto unlock_out;
		}
		kassert( reason == WQUE_RELEASED );
	}

	res->state |= PCACHE_STATE_BUSY;  /* ページを使用中に遷移する  */
	spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);

	mutex_lock(&res->mtx);  /* ページmutexを獲得する */
	if ( ( res->state & ( PCACHE_STATE_VALID | PCACHE_STATE_DIRTY ) ) == 0 ) {

		/* 無効なページキャッシュだった場合は,
		 * ページの内容をロードする
		 */
		kassert_no_reach();
	}

	*pcp = res;

	return 0;

unlock_out:
	spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);

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
pagecache_get(dev_id dev, off_t offset, page_cache *pcp){
	int               rc;
	page_cache       *pc;

	rc = lookup_pagecache(dev, offset, &pc);  /* ページキャッシュを検索する */
	if ( rc == -EINTR )
		return rc;  /* イベント受信 */
	kassert( rc == 0 );  /* TODO: RAMDISKの場合のみ成立するassert */
	/*
	 * TODO: ページキャッシュの割り当て, BUSY状態でキャッシュに登録後,
	 * ページの内容を読み込みVALIDに遷移
	 */

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( pc->state & PCACHE_STATE_BUSY ); /* 自スレッドがページをロックしている */
	kassert( ( pc->state & ( PCACHE_STATE_VALID | PCACHE_STATE_DIRTY ) ) != 0 );
	kassert( ( pc->state & ( PCACHE_STATE_VALID | PCACHE_STATE_DIRTY ) ) 
	    != ( PCACHE_STATE_VALID | PCACHE_STATE_DIRTY ) );
	kassert( pc->bdevid == dev);

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
	kassert( pc->state & PCACHE_STATE_BUSY ); /* 自スレッドがページをロックしている */

	mutex_unlock(&pc->mtx);  /* ページmutexを解放 */

	spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);

	pc->state &= ~PCACHE_STATE_BUSY;  /* ページのロックを解放する */
	wque_wakeup(&pc->waiters, WQUE_RELEASED);  /* ページ待ちスレッドを起床する */

	spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
}

/**
   ページキャッシュをダーティ状態に遷移する
   @param[in]  pc     ページキャッシュ
 */
void
pagecache_mark_dirty(page_cache *pc){

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( pc->state & PCACHE_STATE_BUSY ); /* 自スレッドがページをロックしている */

	/* ページキャッシュのほうが2次記憶より新しい状態となるので
	 * PCACHE_STATE_VALID (2次記憶との同期が完了している)を落とし,
	 * PCACHE_STATE_DIRTY (ページキャッシュのほうが2次記憶より新しい)
	 * を設定する
	 */
	pc->state &= ~PCACHE_STATE_VALID;
	pc->state |= PCACHE_STATE_DIRTY; 
}

/**
   ページキャッシュをディスクに書き戻す
   @param[in]  pc     ページキャッシュ
 */
void
pagecache_write(page_cache *pc){

	/* ページキャッシュmutexを保持していることを確認 */
	kassert( mutex_locked_by_self(&pc->mtx) );
	kassert( pc->state & PCACHE_STATE_BUSY ); /* 自スレッドがページをロックしている */

	mutex_unlock(&pc->mtx);  /* ページmutexを解放 */

	/* TODO: ブロックデバイスへの書き戻し */

	mutex_lock(&pc->mtx);  /* ページmutexを獲得 */

	/* ページキャッシュと2次記憶との同期が完了しているので
	 * PCACHE_STATE_DIRTY (ページキャッシュのほうが2次記憶より新しい)を落とし,
 	 * PCACHE_STATE_VALID (2次記憶との同期が完了している)を設定する
	 */
	pc->state &= ~PCACHE_STATE_DIRTY;
	pc->state |= PCACHE_STATE_VALID;
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
		pc->state = PCACHE_STATE_VALID;  /* キャッシュ読込み済みに設定 */
		/* ページのカーネル仮想アドレスを設定 */
		pc->pc_data = (void *)((uintptr_t)&_fsimg_start + off);
		
		/*
		 * ページキャッシュプールに登録
		 */
		spinlock_lock_disable_intr(&pcache_pool.lock, &iflags);
		res = SPLAY_INSERT(_pcache_tree, &pcache_pool.head, pc);
		spinlock_unlock_restore_intr(&pcache_pool.lock, &iflags);
		kassert(res == NULL);
	}
}
