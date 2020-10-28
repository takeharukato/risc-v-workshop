/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page I/O (page cache) routines                                    */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <kern/mutex.h>
#include <kern/page-if.h>
#include <kern/thr-if.h>
#include <kern/vfs-if.h>
#include <kern/dev-if.h>

#include <fs/vfs/vfs-internal.h>

static kmem_cache page_cache_pool_cache; /**< ページキャッシュプールのSLABキャッシュ */
static kmem_cache page_cache_cache; /**< ページキャッシュのSLABキャッシュ */
/**
   ページキャッシュプールDB赤黒木
 */
static int _pc_ent_cmp(struct _vfs_page_cache *_key, struct _vfs_page_cache *_ent);
RB_GENERATE_STATIC(_vfs_pcache_tree, _vfs_page_cache, pc_ent, _pc_ent_cmp);

/**
   ページキャッシュエントリ比較関数
   @param[in] key 比較対象1
   @param[in] ent RB木内の各エントリ
   @retval 正  keyのオフセットアドレスがentより大きい
   @retval 負  keyのオフセットアドレスがentより小さい
   @retval 0   keyのオフセットアドレスがentに等しい
*/
static int
_pc_ent_cmp(struct _vfs_page_cache *key, struct _vfs_page_cache *ent){

	if ( key->pc_offset > ent->pc_offset )
		return 1;

	if ( key->pc_offset < ent->pc_offset )
		return -1;

	return 0;
}

/**
   ページキャッシュプールを初期化する  (内部関数)
   @param[in] pool ページキャッシュプール
 */
static void
init_page_cache_pool(vfs_page_cache_pool *pool){

	mutex_init(&pool->pcp_mtx);  /* ミューテクスの初期化 */
	refcnt_init(&pool->pcp_refs); /* 参照カウンタの初期化 */
	pool->pcp_state = PCPOOL_DORMANT; /* プールの状態初期化 */
	pool->pcp_bdev = NULL;  /* ブロックデバイスポインタの初期化 */
	pool->pcp_vnode = NULL; /* v-nodeポインタの初期化 */
	pool->pcp_pgsiz = PAGE_SIZE; /* ページサイズの設定 */
	RB_INIT(&pool->pcp_head);  /* ページキャッシュツリーの初期化 */
	queue_init(&pool->pcp_clean_lru);  /* CLEAN LRUの初期化 */
	queue_init(&pool->pcp_dirty_lru);  /* DIRTY LRUの初期化 */
}

/**
   ページキャッシュを初期化する  (内部関数)
   @param[in] pc ページキャッシュ
 */
static void
init_page_cache(vfs_page_cache *pc){

	spinlock_init(&pc->pc_lock); /* スピンロックの初期化 */
	mutex_init(&pc->pc_mtx);  /* ミューテクスの初期化 */
	refcnt_init(&pc->pc_refs); /* 参照カウンタの初期化 */
	pc->pc_pcplink = NULL; /* ページキャッシュプールへのポインタの初期化 */
	pc->pc_state = VFS_PCACHE_INVALID; /* ページキャッシュの状態を初期化 */
	pc->pc_offset = 0;  /* オフセットの初期化 */
	list_init(&pc->pc_lru_link); /* LRUリストエントリの初期化 */
	pc->pc_pf = NULL;            /* ページフレーム情報をNULLに設定 */
	pc->pc_data = NULL;          /* データページをNULLに設定 */
}

/**
   ページキャッシュプールを割り当てる (内部関数)
   @param[in] poolp ページキャッシュプールを指し示すポインタのアドレス
   @retval 0 正常終了
 */
static int
alloc_page_cache_pool(vfs_page_cache_pool **poolp){
	int                    rc;
	vfs_page_cache_pool *pool;

	kassert( poolp != NULL );

	/* ページキャッシュプールを割り当てる */
	rc = slab_kmem_cache_alloc(&page_cache_pool_cache, KMALLOC_NORMAL, (void **)&pool);
	if ( rc != 0 )
		goto error_out;

	init_page_cache_pool(pool); /* ページキャッシュプールを初期化する */

	*poolp = pool;  /* ページキャッシュプールを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ページキャッシュプールを解放する (内部関数)
   @param[in] pool ページキャッシュプール
 */
static void
free_page_cache_pool(vfs_page_cache_pool *pool){

	kassert( refcnt_read(&pool->pcp_refs) == 0 ); /* 参照者がいないことを確認 */
	kassert( pool->pcp_bdev == NULL );  /* ブロックデバイス参照がないことを確認 */
	kassert( pool->pcp_vnode == NULL ); /* v-node参照がないことを確認 */
	kassert( RB_EMPTY(&pool->pcp_head) );  /* プールが空であることを確認 */

	/* LRUが空であることを確認
	 */
	kassert( queue_is_empty(&pool->pcp_clean_lru) );
	kassert( queue_is_empty(&pool->pcp_dirty_lru) );
	/* ページキャッシュプールを解放する */
	slab_kmem_cache_free((void *)pool);

	return ;
}

/**
   ページキャッシュを割り当てる
   @param[in] pcp ページキャッシュを指し示すポインタのアドレス
   @retval    0       正常終了
   @retval   -ENOMEM  メモリ不足
 */
static int
alloc_page_cache(vfs_page_cache **pcp){
	int             rc;
	vfs_page_cache *pc;
	page_frame     *pf;
	void      *newpage;

	kassert( pcp != NULL );

	/* ページキャッシュプールを割り当てる */
	rc = slab_kmem_cache_alloc(&page_cache_cache, KMALLOC_NORMAL, (void **)&pc);
	if ( rc != 0 )
		goto error_out;

	init_page_cache(pc); /* ページキャッシュを初期化する */

	/* ページキャッシュ用のページを割り当てる */
	rc = pgif_get_free_page(&newpage, KMALLOC_NORMAL, PAGE_USAGE_PCACHE);
	if ( rc != 0 ) {

		rc = -ENOMEM;   /* メモリ不足  */
		goto unref_pgcache_out;
	}

	rc = pfdb_kvaddr_to_page_frame(newpage, &pf);
	kassert( rc == 0 ); /* ページプール内のページであるため必ず成功する */

	pc->pc_pf = pf;        /* ページフレーム情報を設定 */
	pc->pc_data = newpage; /* データアドレスを設定 */

	*pcp = pc;  /* ページキャッシュを返却する */

	return 0;

unref_pgcache_out:
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を落として, 解放する */

error_out:
	return rc;
}
/**
   ページキャッシュを解放する (内部関数)
   @param[in] pc ページキャッシュ
 */
static void
free_page_cache(vfs_page_cache *pc){

	kassert( refcnt_read(&pc->pc_refs) == 0 ); /* 参照者がいないことを確認 */
	kassert( pc->pc_pcplink != NULL );  /* ページキャッシュプールへの参照が有効 */
	list_not_linked(&pc->pc_lru_link);  /* LRUにつながっていない */

	wque_wakeup(&pc->pc_waiters, WQUE_DESTROYED);  /* 待ちスレッドを起床 */
	pgif_free_page(pc->pc_data);                   /* ページを解放する */

	/* ページキャッシュの参照が0になるのは, v-nodeのページキャッシュツリーからページを
	 * 取り外して抜ける場合か, ブロックデバイスキャッシュからページを取り外した場合である.
	 * ページキャッシュの参照が0になったら, 本関数が呼ばれ, この時点でページキャッシュへの
	 * 参照が行われない(他スレッドから不可視な状態になる)が保証されるので,
	 * ページキャッシュ解放前に, ページキャッシュプールへの参照を減算する
	 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);
	pc->pc_pcplink = NULL;

	/* ページキャッシュを解放する */
	slab_kmem_cache_free((void *)pc);

	return ;
}

/**
   ページキャッシュを使用中に設定する (内部関数)
   @param[in] pc 操作対象のページキャッシュ
   @retval    0       正常終了
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
static int
mark_page_cache_busy(vfs_page_cache *pc){
	int              rc;
	wque_reason  reason;
	bool          owned;
	bool            res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;  /* 解放中のページキャッシュだった */
		goto error_out;
	}

	/*
	 * ページキャッシュを使用中にする
	 */
	rc = mutex_lock(&pc->pc_mtx);  /* ページキャッシュのmutexロックを獲得する */
	if ( rc != 0 )
		goto unref_pc_out;

	while( VFS_PCACHE_IS_BUSY(pc) ) { /* 他のスレッドがページキャッシュを使用中 */

		/* 他のスレッドがページキャッシュを使い終わるのを待ち合わせる */
		reason = wque_wait_on_queue_with_mutex(&pc->pc_waiters, &pc->pc_mtx);
		if ( reason == WQUE_DELIVEV ) {

			rc = -EINTR;   /* イベントを受信した */
			goto unlock_out;
		}
	}

	pc->pc_state |= VFS_PCACHE_BUSY;  /* ページキャッシュを使用中にする */

	/* オーナをセットする
	 */
	owned = wque_owner_set(&pc->pc_waiters, ti_get_current_thread());
	kassert( owned );

	/* ページキャッシュプールへの参照を獲得 */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert( res ); /* ページキャッシュが空ではないので参照を獲得可能なはず */

	/* LRUにリンクされている場合 */
	if ( !list_not_linked(&pc->pc_lru_link) ){

		/*
		 * LRUから取り外す
		 */
		kassert( VFS_PCACHE_IS_VALID(pc) );

		if ( VFS_PCACHE_IS_CLEAN(pc) )
			queue_del(&pc->pc_pcplink->pcp_clean_lru, &pc->pc_lru_link);
		else
			queue_del(&pc->pc_pcplink->pcp_dirty_lru, &pc->pc_lru_link);
	}

	if ( VFS_PCACHE_IS_VALID(pc) )
		goto success;  /* ページキャッシュ読み込み済み */

	if ( pc->pc_pcplink->pcp_bdev != NULL ) {

		/* TODO: ブロックデバイスからページを読み込む */
		do{}while(0);
		if ( !res ) /* TODO: リードエラー */
			goto unref_page_cache_pool_out;
	} else
		pc->pc_state |= VFS_PCACHE_CLEAN;  /* 有効なページに設定 */

success:
	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	mutex_unlock(&pc->pc_mtx);  /* ページキャッシュのmutexロックを解放する */

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

unref_page_cache_pool_out:
	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

unlock_out:
	mutex_unlock(&pc->pc_mtx);  /* ページキャッシュのロックを解放する */

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

error_out:
	return rc;
}

/**
   ページキャッシュを未使用に設定する (内部関数)
   @param[in] pc 操作対象のページキャッシュ
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
static int
unmark_page_cache_busy(vfs_page_cache *pc){
	int              rc;
	bool            res;
	thread       *owner;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;
		goto error_out;
	}
	/*
	 * ページキャッシュを未使用中にする
	 */
	rc = mutex_lock(&pc->pc_mtx);  /* ページキャッシュのmutexロックを獲得する */
	if ( rc != 0 )
		goto unref_pc_out;

	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* 使用中になっているはず */
	/* BUSYに遷移した時点でページキャッシュ読み込み済みのはず */
	kassert( VFS_PCACHE_IS_VALID(pc) );

	/* 自スレッドがオーナになっていることを確認する
	 */
	owner = wque_owner_get(&pc->pc_waiters);  /* オーナスレッドを獲得 */
	kassert(  owner == ti_get_current_thread() );
	if ( owner != NULL )
		thr_ref_dec(owner);  /* オーナスレッドへの参照を減算 */

	/* LRUにリンクされていないはず */
	kassert( list_not_linked(&pc->pc_lru_link) );

	/* ページキャッシュプールへの参照を獲得 */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert( res ); /* ページキャッシュが空ではないので参照を獲得可能なはず */

	pc->pc_state &= ~VFS_PCACHE_BUSY;  /* ページキャッシュの使用中フラグを落とす */
	/*
	 * LRUの末尾に追加
	 */
	if ( VFS_PCACHE_IS_CLEAN(pc) )
		queue_add(&pc->pc_pcplink->pcp_clean_lru, &pc->pc_lru_link);
	else
		queue_add(&pc->pc_pcplink->pcp_dirty_lru, &pc->pc_lru_link);

	/* オーナをクリアする
	 */
	wque_owner_unset(&pc->pc_waiters);

	wque_wakeup(&pc->pc_waiters, WQUE_RELEASED); /* 待ち合わせ中のスレッドを起床 */

	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	mutex_unlock(&pc->pc_mtx);  /* ページキャッシュのmutexロックを解放する */

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

error_out:
	return rc;
}

/**
   ページキャッシュへの参照を加算する
   @param[in] pc ページキャッシュ
   @retval    真 ページキャッシュの参照を獲得できた
   @retval    偽 ページキャッシュの参照を獲得できなかった
 */
bool
vfs_page_cache_ref_inc(vfs_page_cache *pc){

	/* ページキャッシュ解放中でなければ, 参照カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&pc->pc_refs) != 0 );
}

/**
   ページキャッシュへの参照を減算する
   @param[in] pc ページキャッシュ
   @retval    真 ページキャッシュの最終参照者だった
   @retval    偽 ページキャッシュの最終参照者でない
 */
bool
vfs_page_cache_ref_dec(vfs_page_cache *pc){
	bool res;

	/* ページキャッシュプール解放中でなければ, 参照カウンタを減算
	 */
	res = refcnt_dec_and_test(&pc->pc_refs);
	if ( res )  /* 最終参照者だった場合 */
		free_page_cache(pc);  /* ページキャッシュを解放する */

	return res;
}

/**
   ページキャッシュプールへの参照を加算する
   @param[in] pool ページキャッシュプール
   @retval    真 ページキャッシュプールの参照を獲得できた
   @retval    偽 ページキャッシュプールの参照を獲得できなかった
 */
bool
vfs_page_cache_pool_ref_inc(vfs_page_cache_pool *pool){

	/* ページキャッシュプール解放中でなければ, 参照カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&pool->pcp_refs) != 0 );
}

/**
   ページキャッシュプールへの参照を減算する
   @param[in] pool ページキャッシュプール
   @retval    真 ページキャッシュプールの最終参照者だった
   @retval    偽 ページキャッシュプールの最終参照者でない
 */
bool
vfs_page_cache_pool_ref_dec(vfs_page_cache_pool *pool){
	bool res;

	/* ページキャッシュプール解放中でなければ, 参照カウンタを減算
	 */
	res = refcnt_dec_and_test(&pool->pcp_refs);
	if ( res )  /* 最終参照者だった場合 */
		free_page_cache_pool(pool);  /* ページキャッシュプールを解放する */

	return res;
}

/**
   ブロックデバイスのページキャッシュプールを割り当てる
   @param[in] bdev ブロックデバイスエントリ
   @retval  0      正常終了
   @retval -EBUSY  ページキャッシュプールが既に割り当てられている
   @retval -ENOMEM メモリ不足
 */
int
vfs_dev_page_cache_pool_alloc(bdev_entry *bdev){
	int                    rc;
	vfs_page_cache_pool *pool;

	kassert( bdev != NULL );

	/* ページキャッシュプールを割り当てる */
	rc = alloc_page_cache_pool(&pool);
	if ( rc != 0 )
		goto error_out;

	/* ページキャッシュプールをセットする */
	rc = bdev_page_cache_pool_set(bdev, pool);
	if ( rc != 0 )
		vfs_page_cache_pool_ref_dec(pool); /* 参照を解放し, プールを解放する */

	return 0;

error_out:
	return rc;
}

/**
   ページキャッシュプールからページキャッシュを検索し参照を得る
   @param[in] pool ページキャッシュプール
   @param[in] offset オフセットアドレス(単位:バイト)
   @param[out] pcp   ページキャッシュを指し示すポインタのアドレス
   @retval  0 正常終了
   @retval -ENODEV  ミューテックスが破棄された
   @retval -EINTR   非同期イベントを受信した
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_get(vfs_page_cache_pool *pool, off_t offset, vfs_page_cache **pcp){
	int              rc;
	vfs_page_cache  key;
	vfs_page_cache  *pc;
	vfs_page_cache *res;
	bool            ref;

	kassert( pcp != NULL );

	ref = vfs_page_cache_pool_ref_inc(pool); /* ページキャッシュプールの参照を獲得する */
	kassert( ref );

	rc = mutex_lock(&pool->pcp_mtx);  /* ページキャッシュプールのロックを獲得する */
	if ( rc != 0 )
		goto put_pool_ref_out;

	/* オフセットをページキャッシュ境界に合わせる */
	key.pc_offset = truncate_align(offset, pool->pcp_pgsiz);
	pc = RB_FIND(_vfs_pcache_tree, &pool->pcp_head, &key);
	if ( pc != NULL )
		goto found; /* ページキャッシュが見つかった */

	rc = alloc_page_cache(&pc);  /* ページキャッシュを新規に割り当てる */
	if ( rc != 0 )
		goto unlock_out;

	pc->pc_pcplink = pool;  /* ページプールを設定 */
	/* ページオフセットアドレスを設定 */
	pc->pc_offset = truncate_align(offset, pool->pcp_pgsiz);

	/* ページキャッシュを登録 */
	res = RB_INSERT(_vfs_pcache_tree, &pool->pcp_head, pc);
	kassert( res == NULL );
	vfs_page_cache_pool_ref_inc(pool);  /* ページキャッシュからの参照を加算 */

found:
	ref = vfs_page_cache_ref_inc(pc);  /* スレッドからの参照を加算 */
	kassert( res );  /* プール内に登録されているので参照を獲得可能 */

	mutex_unlock(&pool->pcp_mtx);   /* ページキャッシュプールのロックを解放する */

	vfs_page_cache_pool_ref_dec(pool); /* ページキャッシュプールの参照を解放する */

	rc = mark_page_cache_busy(pc); /* ページキャッシュを使用中に遷移する */
	if ( rc != 0 ) {  /* ページキャッシュを使用中に遷移できなかった */

		vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */
		goto error_out;
	}

	*pcp = pc;  /* ページキャッシュを返却 */

	return 0;

unlock_out:
	mutex_unlock(&pool->pcp_mtx);   /* ページキャッシュプールのロックを解放する */

put_pool_ref_out:
	vfs_page_cache_pool_ref_dec(pool); /* ページキャッシュプールの参照を解放する */

error_out:
	return rc;
}
/**
   ページキャッシュを返却する
   @param[in] pc ページキャッシュ
   @retval  0 正常終了
   @retval -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_put(vfs_page_cache *pc){
	int              rc;
	bool            res;

	res = vfs_page_cache_ref_inc(pc);  /* 操作用にページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;
		goto error_out;
	}

	rc = unmark_page_cache_busy(pc);  /* ページキャッシュを未使用状態に遷移する */

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュの参照を解放 */

	if ( rc != 0 )
		goto error_out;

	return 0;

error_out:
	return rc;
}

/**
   ページI/O機構を初期化する
 */
void
vfs_init_pageio(void){
	int rc;

	/* ページキャッシュプールのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&page_cache_pool_cache, "Page cache pool cache",
	    sizeof(vfs_page_cache_pool), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* ページキャッシュのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&page_cache_cache, "Page cache cache",
	    sizeof(vfs_page_cache), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   ページI/O機構を終了する
 */
void
vfs_finalize_pageio(void){

	/* ページキャッシュプールのキャッシュを解放する */
	slab_kmem_cache_destroy(&page_cache_pool_cache);
	/* ページキャッシュのキャッシュを解放する */
	slab_kmem_cache_destroy(&page_cache_cache);
}
