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
#include <kern/vfs-if.h>
#include <kern/dev-if.h>

#include <fs/vfs/vfs-internal.h>

/** ページキャッシュプールDB */
static vfs_page_cache_pool_db __unused g_pcpdb = __PCPDB_INITIALIZER(&g_pcpdb);

static kmem_cache page_cache_pool_cache; /**< ページキャッシュプールのSLABキャッシュ */

/**
   ページキャッシュプールを初期化する  (内部関数)
   @param[in] pool ページキャッシュプール
 */
static void __unused
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
vfs_dev_page_cache_alloc(bdev_entry *bdev){
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
   ページI/O機構を初期化する
 */
void
vfs_init_pageio(void){
	int rc;

	/* ページキャッシュプールのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&page_cache_pool_cache, "Page cache pool cache",
	    sizeof(vfs_page_cache_pool), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   ページI/O機構を終了する
 */
void
vfs_finalize_pageio(void){

	/* ページキャッシュプールのキャッシュを解放する */
	slab_kmem_cache_destroy(&page_cache_pool_cache);
}
