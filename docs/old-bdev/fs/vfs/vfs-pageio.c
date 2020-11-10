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
	pool->pcp_bdevid = VFS_VSTAT_INVALID_DEVID;  /* ブロックデバイスIDの初期化 */
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
	wque_init_wait_queue(&pc->pc_waiters);  /* 待ちキューの初期化 */
	list_init(&pc->pc_lru_link); /* LRUリストエントリの初期化 */
	queue_init(&pc->pc_buf_que); /* ブロックバッファキューの初期化 */
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
	kassert( RB_EMPTY(&pool->pcp_head) );  /* プールが空であることを確認 */

	/* ブロックデバイスの設定を解除 */
	pool->pcp_bdevid = VFS_VSTAT_INVALID_DEVID;
	pool->pcp_state = PCPOOL_DELETE; /* プール削除中 */

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
	bool           res;
	vfs_page_cache *pc;
	page_frame     *pf;
	void      *newpage;

	kassert( pcp != NULL );

	/* ページキャッシュを割り当てる */
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
	res = vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を落として, 解放する */
	kassert( res );  /* プールに登録する前なので, 最終参照者であるはず */

error_out:
	return rc;
}

/**
   ページキャッシュからブロックバッファを取り出す (内部関数)
   @param[in]   pc    ページキャッシュ
   @param[out]  bufp  ブロックバッファを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOENT ページキャッシュが解放中だった
   @retval -EAGAIN ブロックバッファが登録されていない
 */
static int
dequeue_block_buffer_from_page_cache(vfs_page_cache *pc, block_buffer **bufp){
	int            rc;
	block_buffer *buf;
	intrflags  iflags;

	kassert( bufp != NULL );

	spinlock_lock_disable_intr(&pc->pc_lock, &iflags);

	if ( queue_is_empty(&pc->pc_buf_que) ) {  /* キューが空の場合 */

		rc = -EAGAIN; /* ブロックバッファが登録されていない */
		goto unlock_out;
	}

	/* 先頭のブロックバッファを取り出す */
	buf = container_of(queue_get_top(&pc->pc_buf_que), block_buffer, b_ent);

	kassert( buf->b_page == pc );
	buf->b_page = NULL;  /* ページキャッシュへのリンクを解除 */

	*bufp = buf;  /* ブロックバッファを返却する */

	spinlock_unlock_restore_intr(&pc->pc_lock, &iflags);

	return 0;

unlock_out:
	spinlock_unlock_restore_intr(&pc->pc_lock, &iflags);

	return rc;
}

/**
   ページキャッシュ中のブロックバッファを解放する(内部関数)
   @param[in] pc ページキャッシュ
 */
static void
unmap_block_buffers(vfs_page_cache *pc){
	int                rc;
	block_buffer *cur_buf;

	/* 先頭のバッファを取り出す */
	rc = dequeue_block_buffer_from_page_cache(pc, &cur_buf);
	while( rc == 0 ) {

		block_buffer_free(cur_buf); /* バッファを解放する */

		/* 先頭のバッファを取り出す */
		rc = dequeue_block_buffer_from_page_cache(pc, &cur_buf);
		if ( rc == -EAGAIN )
			break;  /* キューにバッファがない */
		kassert( rc == 0 );
	}
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

	/* ページキャッシュへの参照をとらずにブロックバッファを解放する */
	if ( !queue_is_empty( &pc->pc_buf_que ) )
		unmap_block_buffers(pc);  /* ブロックバッファを解放 */

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
   ロックを取らずにページキャッシュを使用中に設定する (内部関数)
   @param[in] pc 操作対象のページキャッシュ
   @retval    0       正常終了
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
static int
mark_page_cache_busy_nolock(vfs_page_cache *pc){
	bool            res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュの参照を獲得 */
	if ( !res )
		return -ENOENT;  /* 解放中のページキャッシュだった */

	pc->pc_state |= VFS_PCACHE_BUSY;  /* ページキャッシュを使用中にする */

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;
}

/**
   ロックを取らずにページキャッシュを未使用中に設定する (内部関数)
   @param[in] pc 操作対象のページキャッシュ
   @retval    0       正常終了
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
static int
unmark_page_cache_busy_nolock(vfs_page_cache *pc){
	bool            res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュの参照を獲得 */
	if ( !res )
		return -ENOENT;  /* 解放中のページキャッシュだった */

	pc->pc_state &= ~VFS_PCACHE_BUSY;  /* ページキャッシュを使用中にする */

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;
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

	/* ページキャッシュプールへの参照を獲得 */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert( res ); /* ページキャッシュが空ではないので参照を獲得可能なはず */

	/* ページキャッシュプールのmutexロックを獲得する */
	rc = mutex_lock(&pc->pc_pcplink->pcp_mtx);
	if ( rc != 0 )
		goto unref_pcp_out;

	/*
	 * ページキャッシュを使用中にする
	 */
	while( VFS_PCACHE_IS_BUSY(pc) ) { /* 他のスレッドがページキャッシュを使用中 */

		/* 他のスレッドがページキャッシュを使い終わるのを待ち合わせる */
		reason = wque_wait_on_queue_with_mutex(&pc->pc_waiters,
		    &pc->pc_pcplink->pcp_mtx);
		if ( reason == WQUE_DELIVEV ) {

			rc = -EINTR;   /* イベントを受信した */
			goto unlock_out;
		}

		if ( reason == WQUE_LOCK_FAIL ) {

			/* mutex獲得に失敗したためページキャッシュ獲得を断念 */
			rc = -EINTR;   /* イベントを受信した */
			goto unref_pcp_out;
		}
	}

	mark_page_cache_busy_nolock(pc);  /* ページキャッシュを使用中にする */

	/* オーナをセットする
	 */
	owned = wque_owner_set(&pc->pc_waiters, ti_get_current_thread());
	kassert( owned );

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

	/* ブロックデバイスのキャッシュでない場合, 状態を読み込み済みに設定する
	 */
	if ( pc->pc_pcplink->pcp_bdevid == VFS_VSTAT_INVALID_DEVID )
		pc->pc_state |= VFS_PCACHE_CLEAN;  /* 読み込み済みページに設定 */

success:
	/* ページキャッシュプールのロックを解放する */
	mutex_unlock(&pc->pc_pcplink->pcp_mtx);

	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

unlock_out:
	/* ページキャッシュプールのロックを解放する */
	mutex_unlock(&pc->pc_pcplink->pcp_mtx);

unref_pcp_out:
	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

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

	kassert( pc->pc_pcplink != NULL );
	/* ページキャッシュプールの参照を得る */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert( res ); /* ページキャッシュが空ではないので参照を獲得可能なはず */

	/* ページキャッシュプールのmutexロックを獲得する */
	rc = mutex_lock(&pc->pc_pcplink->pcp_mtx);
	if ( rc != 0 )
		goto unref_pcp_out;

	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* 使用中になっているはず */

	/* 自スレッドがオーナになっていることを確認する
	 */
	owner = wque_owner_get(&pc->pc_waiters);  /* オーナスレッドを獲得 */
	kassert(  owner == ti_get_current_thread() );
	if ( owner != NULL )
		thr_ref_dec(owner);  /* オーナスレッドへの参照を減算 */

	/* LRUにリンクされていないはず */
	kassert( list_not_linked(&pc->pc_lru_link) );

	unmark_page_cache_busy_nolock(pc);  /* ページキャッシュの使用中フラグを落とす */

	/* BUSY中に読み込みまたは書き込みを行っているはず */
	kassert( VFS_PCACHE_IS_VALID(pc) );

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

	/* ページキャッシュプールのmutexロックを解放する */
	mutex_unlock(&pc->pc_pcplink->pcp_mtx);

	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

unref_pcp_out:
	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

error_out:
	return rc;
}

/**
   ページキャッシュの状態を更新する (内部関数)
   @param[in] pc        操作対象のページキャッシュ
   @param[in] new_state 更新後のページキャッシュの状態
   @retval    0         正常終了
   @retval   -EINVAL    不正な状態を指定した
   @retval   -ENOENT    ページキャッシュが解放中だった
 */
static int
change_page_cache_state(vfs_page_cache *pc, pcache_state new_state){
	int              rc;
	bool            res;
	pcache_state    new;

	new = new_state & VFS_PCACHE_STATE_MASK;  /* 状態更新マスク */
	if ( ( new != VFS_PCACHE_CLEAN ) && ( new != VFS_PCACHE_DIRTY ) )
		return -EINVAL; /* 不正な状態 */

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	/*
	 * ページキャッシュを読み込み済みにする
	 */
	rc = mutex_lock(&pc->pc_mtx);  /* ページキャッシュのmutexロックを獲得する */
	if ( rc != 0 )
		goto unref_pc_out;

	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* 使用中になっているはず */

	/* ページキャッシュの更新済みフラグ/ダーティフラグを落とす */
	pc->pc_state &= ~VFS_PCACHE_STATE_MASK;
	pc->pc_state |= new;  /* ページキャッシュの状態を更新 */

	mutex_unlock(&pc->pc_mtx);  /* ページキャッシュのmutexロックを解放する */

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

error_out:
	return rc;
}


/**
   ページキャッシュを無効にする (内部関数)
   @param[in] pc 操作対象のページキャッシュ
   @retval    0      正常終了
   @retval   -EINVAL ページキャッシュのデバイスIDが不正
   @retval   -ENODEV 指定されたデバイスが見つからなかった
   @retval   -ENOENT ページキャッシュが解放中だった
   @retval   -EIO    ページの書き出しに失敗した
   @note ページキャッシュプールのmutexを獲得してから呼び出す
 */
static int
invalidate_page_cache_common(vfs_page_cache *pc){
	int                 rc;
	bool               res;
	dev_id           devid;
	vfs_page_cache *pc_res;

	res = vfs_page_cache_ref_inc(pc);  /* 操作用にページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;  /* 解放中のページキャッシュだった */
		goto error_out;
	}

	/* 操作用にページキャッシュプールへの参照を獲得 */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert( res ); /* ページキャッシュが空ではないので参照を獲得可能なはず */

	devid = pc->pc_pcplink->pcp_bdevid;  /* デバイスIDを獲得 */

	kassert( VFS_PCACHE_IS_VALID(pc) );
	kassert( VFS_PCACHE_IS_BUSY(pc) );

	/* ページキャッシュの方が新しければページキャッシュを書き戻す */
	if ( VFS_PCACHE_IS_VALID(pc) && VFS_PCACHE_IS_DIRTY(pc) ) {

		rc = vfs_page_cache_rw(pc);  /* 二次記憶と同期する */
		if ( rc != 0 )
			kprintf("Error vfs pageio: devid: 0x%qx page cache: %p "
			    "I/O fail rc=%d\n", devid, pc, rc);
	}

	/* ページキャッシュプールから取り外す
	 * ページキャッシュの参照数が0になった時点でfree_page_cacheで
	 * ページキャッシュからページキャッシュプールへの参照を減算するので
	 * 本関数ではページキャッシュからページキャッシュプールへの参照は減算しない
	 */
	pc_res = RB_REMOVE(_vfs_pcache_tree, &pc->pc_pcplink->pcp_head, pc);
	kassert( pc_res != NULL );

	/* LRUから取り除く */
	if ( VFS_PCACHE_IS_CLEAN(pc) )
		queue_del(&pc->pc_pcplink->pcp_clean_lru, &pc->pc_lru_link);
	else
		queue_del(&pc->pc_pcplink->pcp_dirty_lru, &pc->pc_lru_link);

	/* 操作用に獲得したページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュプールからの参照を減算して解放する */

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

error_out:
	return rc;
}

/**
   LRU内のページを回収する (内部関数)
   @param[in]  lru         回収対象ページキャッシュプールのLRU
   @param[in]  reclaim_nr  回収ページ数(単位:個, 負の場合は, 全ページの回収を試みる)
   @param[out] reclaimedp  回収したページ数を返却する領域
   @retval     0           正常終了
   @retval    -EBUSY       回収できなかったページがある
*/
static int
shrink_page_caches_in_lru_nolock(queue *lru, singned_cnt_type reclaim_nr,
    singned_cnt_type *reclaimedp){
	int                     rc;
	bool                   res;
	vfs_page_cache         *pc;
	list              *lp, *np;
	singned_cnt_type reclaimed;
	singned_cnt_type   remains;
	singned_cnt_type  fail_cnt;

	remains =  reclaim_nr;  /* 残り回収ページ数 */
	reclaimed = 0; /* 回収済みページ数を初期化 */
	fail_cnt = 0;  /* 回収失敗ページ数 */

	/* LRU内のページキャッシュを回収する
	 */
	queue_for_each_safe(lp, lru, np) {

		if ( remains == 0)
			break;  /* 回収ページ数に達した */

		/* ページキャッシュを参照 */
		pc = container_of(lp, vfs_page_cache, pc_lru_link);

		/* 操作用にページキャッシュの参照を獲得 */
		res = vfs_page_cache_ref_inc(pc);
		if ( !res )
			continue;  /* 次のページを処理する */

		/* LRUにつながっているので, 使用中ではないはず */
		kassert( !VFS_PCACHE_IS_BUSY(pc) );

		/* lpが指し示しているページキャッシュを無効化する
		 */
		mark_page_cache_busy_nolock(pc);  /* ページキャッシュを使用中にする */

		rc = invalidate_page_cache_common(pc);  /* ページキャッシュを無効化する */
		if ( rc != 0 ) { /* ページを無効化できなかった場合は次のページを処理する */

			unmark_page_cache_busy_nolock(pc); /* 使用権を返却する */
			/* ページキャッシュの参照を得ているので-ENOENTは返らない */
			kassert( rc != -ENOENT );

			/* LRUにリンクされているはず */
			kassert( !list_not_linked(&pc->pc_lru_link) );
			++fail_cnt; /* 回収失敗ページ数を加算 */
			vfs_page_cache_ref_dec(pc); /* ページキャッシュへの参照を返却する */
			continue;
		}

		vfs_page_cache_ref_dec(pc); /* ページキャッシュへの参照を返却する */

		++reclaimed;  /* 回収したページ数を加算 */

		if ( queue_is_empty(lru) )
			break; /* キューが空になったら抜ける */

		if ( reclaim_nr > 0 )
			--remains;  /* 残回収ページ数を減算 */
	}

	if ( reclaimedp != NULL )
		*reclaimedp = reclaimed; /* 回収済みページ数を返却 */

	if ( fail_cnt > 0 )
		return -EBUSY;  /* 回収できなかったページがある */

	return 0;
}

/*
 * IF関数
 */

/**
   ページキャッシュを無効にする
   @param[in] pc 操作対象のページキャッシュ(使用権獲得済みのページキャッシュ)
   @retval    0       正常終了
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_invalidate(vfs_page_cache *pc){
	int   rc;
	bool res;

	res = vfs_page_cache_ref_inc(pc);  /* 操作用にページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;  /* 解放中のページキャッシュだった */
		goto error_out;
	}

	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* ページキャッシュの使用権を得ていること */

	/* ページキャッシュプールの参照を獲得する */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	if ( !res ) {

		rc = -ENOENT;  /* 解放中のページキャッシュプールだった */
		goto unref_pc_out;
	}

	/* ページキャッシュプールのmutexロックを獲得する */
	rc = mutex_lock(&pc->pc_pcplink->pcp_mtx);
	if ( rc != 0 )
		goto unref_pcp_out;

	rc = invalidate_page_cache_common(pc);  /* ページキャッシュを無効化する */
	if ( rc != 0 )
		goto unlock_pcp_out;

	/* ページキャッシュプールのロックを解放する */
	mutex_unlock(&pc->pc_pcplink->pcp_mtx);

	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* スレッドからの参照を減算 */

	return 0;

unlock_pcp_out:
	/* ページキャッシュプールのロックを解放する */
	mutex_unlock(&pc->pc_pcplink->pcp_mtx);

unref_pcp_out:
	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

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
   ページキャッシュプール内のページを回収する
   @param[in] pool 操作対象のページキャッシュプール
   @param[in] reclaim_nr 回収ページ数(単位:個, 負の値を指定した場合は, 全ページの回収を試みる)
   @param[out] reclaimedp 回収したページ数を返却する領域
   @retval  0      正常終了
   @retval -ENOENT 解放中のページキャッシュプールだった
   @retval -EBUSY  未回収ページがある
 */
int
vfs_page_cache_pool_shrink(vfs_page_cache_pool *pool, singned_cnt_type reclaim_nr,
    singned_cnt_type *reclaimedp){
	int                             rc;
	bool                           res;
	singned_cnt_type   total_reclaimed;
	singned_cnt_type         reclaimed;
	singned_cnt_type           remains;
	bool                reclaim_failed;

	reclaim_failed = false;  /* 未回収ページなしに初期化 */

	res = vfs_page_cache_pool_ref_inc(pool); /* ページキャッシュプールの参照を獲得する */
	if ( !res )
		return -ENOENT;  /* 解放中のページキャッシュプール */

	rc = mutex_lock(&pool->pcp_mtx);  /* ページキャッシュプールのロックを獲得する */
	if ( rc != 0 )
		goto unref_pcp_out;

	total_reclaimed = 0;   /* 総回収済みページ数 */
	remains = reclaim_nr;  /* 回収目標ページ数 */

	/* ダーティページの場合, 回収に先んじてキャッシュを二次記憶に書き戻す必要があり,
	 * 回収に時間がかかることから, より短時間で回収可能なページキャッシュと二次記憶間
	 * での一貫性がとれているページから回収を試みる
	 */
	rc = shrink_page_caches_in_lru_nolock(&pool->pcp_clean_lru, remains, &reclaimed);
	if ( reclaim_nr > 0 ) {

		kassert( remains >= reclaimed );
		remains -= reclaimed;  /* 残回収ページ数を減算 */
	}
	if ( rc == -EBUSY )
		reclaim_failed = true;  /* 未回収ページあり */

	total_reclaimed += reclaimed;  /* 回収済みページ数を加算 */

	if ( remains == 0 )
		goto done; /* 回収目標を達成 */

	/* ダーティページの回収を試みる
	 */
	rc = shrink_page_caches_in_lru_nolock(&pool->pcp_dirty_lru, remains, &reclaimed);
	if ( reclaim_nr > 0 ) {

		kassert( remains >= reclaimed );
		remains -= reclaimed;  /* 残回収ページ数を減算 */
	}

	if ( rc == -EBUSY )
		reclaim_failed = true;  /* 未回収ページあり */

	total_reclaimed += reclaimed;  /* 回収済みページ数を加算 */

done:
	mutex_unlock(&pool->pcp_mtx);  /* ページキャッシュプールのロックを解放する */

	vfs_page_cache_pool_ref_dec(pool); /* ページキャッシュプールの参照を減算する */

	if ( reclaimedp != NULL )
		*reclaimedp = total_reclaimed; /* 総回収ページ数を返却 */

	if ( reclaim_failed ) {

		rc = -EBUSY;  /* 未回収ページあり */
		goto error_out;
	}

	return 0;

unref_pcp_out:
	vfs_page_cache_pool_ref_dec(pool); /* ページキャッシュプールの参照を減算する */

error_out:
	return rc;
}

/**
   ページキャッシュプール内のページサイズを獲得する
   @param[in]  pool      操作対象のページキャッシュプール
   @param[out] pagesizep ページサイズ返却領域
   @retval  0      正常終了
 */
int
vfs_page_cache_pool_pagesize_get(vfs_page_cache_pool *pool, size_t *pagesizep){
	int          rc;
	bool        res;
	size_t pagesize;

	res = vfs_page_cache_pool_ref_inc(pool); /* ページキャッシュプールの参照を獲得する */
	if ( !res )
		return -ENOENT;  /* 解放中のページキャッシュプール */

	rc = mutex_lock(&pool->pcp_mtx);  /* ページキャッシュプールのロックを獲得する */
	if ( rc != 0 )
		goto unref_pcp_out;

	pagesize = pool->pcp_pgsiz; /* ページサイズ獲得 */

	mutex_unlock(&pool->pcp_mtx);  /* ページキャッシュプールのロックを解放する */

	vfs_page_cache_pool_ref_dec(pool); /* ページキャッシュプールの参照を減算する */

	if ( pagesizep != NULL )
		*pagesizep = pagesize; /* ページサイズを返却 */

	return 0;

unref_pcp_out:
	vfs_page_cache_pool_ref_dec(pool); /* ページキャッシュプールの参照を減算する */

	return rc;
}

/**
   ページキャッシュを読み込み済みに設定する
   @param[in] pc 操作対象のページキャッシュ
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_mark_clean(vfs_page_cache *pc){

	/* ページキャッシュを読み込み済みに遷移する */
	return change_page_cache_state(pc, VFS_PCACHE_CLEAN);
}

/**
   ページキャッシュを更新済みに設定する
   @param[in] pc 操作対象のページキャッシュ
   @retval   -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_mark_dirty(vfs_page_cache *pc){

	/* ページキャッシュを更新済みに遷移する */
	return change_page_cache_state(pc, VFS_PCACHE_DIRTY);
}

/**
   ファイル(v-node)のページキャッシュプールを割り当てる
   @param[in] bdev  ブロックデバイスエントリ
   @retval  0       正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   @retval -EBUSY  ページキャッシュプールが既に割り当てられている
   @retval -ENOMEM メモリ不足
   @note ブロックデバイス登録処理から呼び出されるページキャッシュ機構の内部のIF関数であるため,
   登録前のブロックデバイスエントリを引数にとる
 */
int
vfs_vnode_page_cache_pool_alloc(vnode *v){
	int                    rc;
	vfs_page_cache_pool *pool;

	/* ページキャッシュプールを割り当てる */
	rc = alloc_page_cache_pool(&pool);
	if ( rc != 0 )
		goto error_out;

	kassert( pool->pcp_bdevid == VFS_VSTAT_INVALID_DEVID );

	/* ページキャッシュプールをセットする */
	v->v_pcp = pool;

	return 0;

error_out:
	return rc;
}

/**
   ブロックデバイスのページキャッシュプールを割り当てる
   @param[in] bdev  ブロックデバイスエントリ
   @retval  0       正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   @retval -EBUSY  ページキャッシュプールが既に割り当てられている
   @retval -ENOMEM メモリ不足
   @note ブロックデバイス登録処理から呼び出されるページキャッシュ機構の内部のIF関数であるため,
   登録前のブロックデバイスエントリを引数にとる
 */
int
vfs_dev_page_cache_pool_alloc(bdev_entry *bdev){
	int                    rc;
	bool                  res;
	vfs_page_cache_pool *pool;

	/* ページキャッシュプールを割り当てる */
	rc = alloc_page_cache_pool(&pool);
	if ( rc != 0 )
		goto error_out;

	/* ページキャッシュプールをセットする */
	rc = bdev_page_cache_pool_set(bdev, pool);
	if ( rc != 0 ) {

		res = vfs_page_cache_pool_ref_dec(pool); /* 参照を解放し, プールを解放する */
		kassert( res );  /* 最終参照者であるはず */
	}

	return 0;

error_out:
	return rc;
}

/**
   ページキャッシュのデバイスIDを得る
   @param[in]  pc     ページキャッシュ
   @param[out] devidp ページサイズを返却する領域
   @retval  0      正常終了
   @retval -ENODEV ブロックデバイス上のページキャッシュではない
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
vfs_page_cache_devid_get(vfs_page_cache *pc, dev_id *devidp){
	int       rc;
	bool     res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュへの参照を得る */
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	kassert( pc->pc_pcplink != NULL ); /* ページキャッシュプールにつながっている */

	if ( !VFS_PCACHE_IS_DEVICE_PAGE(pc) ) {

		rc = -ENODEV; /* ブロックデバイス上のページキャッシュではない */
		goto unref_pc_out;
	}

	if ( devidp != NULL )
		*devidp = pc->pc_pcplink->pcp_bdevid; /* デバイスIDを返却する */

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return 0;

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

error_out:
	return rc;
}

/**
   ページキャッシュのページサイズを得る
   @param[in]  pc    ページキャッシュ
   @param[out] sizep ページサイズを返却する領域
   @retval  0      正常終了
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
vfs_page_cache_pagesize_get(vfs_page_cache *pc, size_t *sizep){
	int          rc;
	bool        res;
	size_t pagesize;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュへの参照を得る */
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	kassert( pc->pc_pcplink != NULL ); /* ページキャッシュプールにつながっている */

	/* ページサイズ取得 */
	rc = vfs_page_cache_pool_pagesize_get(pc->pc_pcplink, &pagesize);
	if ( rc != 0 )
		goto unref_pc_out;

	if ( sizep != NULL )
		*sizep = pagesize; /* ページサイズを返却する */

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return 0;

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

error_out:
	return rc;
}
/**
   ページキャッシュのデータ領域を得る
   @param[in]  pc    ページキャッシュ
   @param[out] datap データ領域を指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
vfs_page_cache_refer_data(vfs_page_cache *pc, void **datap){
	int       rc;
	bool     res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュへの参照を得る */
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	if ( datap != NULL )
		*datap = pc->pc_data; /* データ領域を返却する */

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return 0;

error_out:
	return rc;
}

/**
   ページキャッシュにブロックバッファを追加する
   @param[in]  pc   ページキャッシュ
   @param[in]  buf  ブロックバッファ
   @retval  0      正常終了
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
vfs_page_cache_enqueue_block_buffer(vfs_page_cache *pc, block_buffer *buf){
	int           rc;
	bool         res;
	intrflags iflags;

	res = vfs_page_cache_ref_inc(pc); /* ページキャッシュへの参照を得る */
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	spinlock_lock_disable_intr(&pc->pc_lock, &iflags);
	kassert( buf->b_page == NULL );  /* ページキャッシュ未設定 */
	queue_add(&pc->pc_buf_que, &buf->b_ent); /* ページキャッシュに追加 */
	buf->b_page = pc;  /* ページキャッシュへのリンクを設定 */
	spinlock_unlock_restore_intr(&pc->pc_lock, &iflags);

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return 0;

error_out:
	return rc;
}

/**
   ページキャッシュ中のブロックバッファを解放する
   @param[in] pc ページキャッシュ
 */
void
vfs_page_cache_block_buffer_unmap(vfs_page_cache *pc){
	bool res;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュへの参照を得る */
	kassert( res ); /*  ページキャッシュへの参照を獲得済み  */

	unmap_block_buffers(pc);  /* ページキャッシュ中のブロックバッファを解放する */

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return;
}
/**
   ページキャッシュ中のブロックバッファを検索する
   @param[in]   pc     ページキャッシュ
   @param[in]   offset ページ内オフセットアドレス
   @param[out]  bufp  ブロックバッファを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL オフセットがページサイズを超えている
   @retval -ENOENT ページキャッシュが解放中だった
 */
int
vfs_page_cache_block_buffer_find(vfs_page_cache *pc, size_t offset, block_buffer **bufp){
	int            rc;
	bool          res;
	list         *cur;
	block_buffer *buf;
	size_t      pgsiz;
	intrflags  iflags;

	res = vfs_page_cache_ref_inc(pc);
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	rc = vfs_page_cache_pagesize_get(pc, &pgsiz); /* ページサイズを得る */
	if ( rc != 0 )
		goto unref_pc_out;

	kassert( VFS_PCACHE_IS_BUSY(pc) ); /* ページキャッシュの使用権を得ていることを確認 */

	if ( offset >= pgsiz ) {

		rc = -EINVAL;  /* オフセットがページサイズを超えている */
		goto unref_pc_out;
	}

	/* ブロックデバイスのページキャッシュであることを確認
	 */
	kassert( VFS_PCACHE_IS_DEVICE_PAGE(pc) );

	spinlock_lock_disable_intr(&pc->pc_lock, &iflags); /* ページキャッシュロックを獲得 */

	/* ページキャッシュ中のブロックバッファを検索する
	 */
	queue_for_each(cur, &pc->pc_buf_que){

		buf = container_of(cur, block_buffer, b_ent); /* ブロックバッファを参照 */

		/* ブロックバッファサイズが正しいことを確認 */
		kassert( VFS_PCACHE_BUFSIZE_VALID(buf->b_len, pgsiz) );

		if ( ( buf->b_offset <= offset )
		    && ( offset < ( buf->b_offset + buf->b_len ) ) )
			goto found;  /* ブロックキャッシュを見つけた */
	}

	rc = -ENOENT;  /* ブロックバッファが見つからなかった */
	goto unlock_out;

found:
	if ( bufp != NULL )
		*bufp = buf;  /* バッファキャッシュを返却 */

	/* ページキャッシュロックを解放 */
	spinlock_unlock_restore_intr(&pc->pc_lock, &iflags);
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return 0;

unlock_out:
	/* ページキャッシュロックを解放 */
	spinlock_unlock_restore_intr(&pc->pc_lock, &iflags);

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

error_out:
	return rc;
}

/**
   ページキャッシュにブロックバッファが含まれていないことを確認する
   @param[in] pc ページキャッシュ
   @retval    真 ページキャッシュにブロックバッファが含まれていない
   @retval    偽 ページキャッシュにブロックバッファが含まれている
   @note ブロックデバイス処理から呼ばれるvfs<->ブロックデバイス間IF
 */
bool
vfs_page_cache_is_block_buffer_empty(vfs_page_cache *pc){
	bool          res;
	intrflags  iflags;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュの参照を獲得 */
	if ( !res ) {

		res = true;  /* 解放中のページキャッシュだった */
		goto error_out;
	}

	spinlock_lock_disable_intr(&pc->pc_lock, &iflags); /* ページキャッシュロックを獲得 */

	res = queue_is_empty(&pc->pc_buf_que); /* キューが空であることを確認 */

	/* ページキャッシュロックを解放 */
	spinlock_unlock_restore_intr(&pc->pc_lock, &iflags);

	vfs_page_cache_ref_dec(pc);
error_out:
	return res;
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
   @retval -ENOENT  ページキャッシュプール/ページキャッシュが解放中だった
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
	if ( !ref )
		return -ENOENT;  /* 開放中のページキャッシュプール */

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
	kassert( ref );  /* プール内に登録されているので参照を獲得可能 */

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

	vfs_page_cache_ref_dec(pc);  /* 操作用のページキャッシュの参照を解放 */

	if ( rc != 0 )
		goto error_out;

	return 0;

error_out:
	return rc;
}

/**
   ブロックデバイス上のページキャッシュの読み込み/書き込みを行う
   @param[in] pc ページキャッシュ
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   @retval -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_rw(vfs_page_cache *pc){
	int              rc;
	bool            res;
	dev_id        devid;
	bdev_entry    *bdev;

	res = vfs_page_cache_ref_inc(pc);  /* 操作用にページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;
		goto error_out;
	}

	/* ブロックデバイスのページキャッシュであることを確認
	 */
	kassert( VFS_PCACHE_IS_DEVICE_PAGE(pc) );
	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* 使用権があることを確認 */

	/* 二次記憶との一貫性が保たれている場合はI/Oをスキップする */
	if ( VFS_PCACHE_IS_VALID(pc) && !VFS_PCACHE_IS_DIRTY(pc) )
		goto io_finished;

	/* ページの読み書きを行う
	 */
	rc = vfs_page_cache_devid_get(pc, &devid);  /* デバイスIDを取得 */
	kassert( rc == 0 );

	/* ブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(devid, &bdev);
	if ( rc != 0 )
		goto unref_pc_out;

	if ( bdev->bdent_calls.fs_strategy == NULL )
		goto no_real_device;  /* 実デバイスへの読み書き不要 */

	rc = bdev->bdent_calls.fs_strategy(devid, pc); /* ページの内容を読み書きする */
	if ( rc != 0 )
		goto put_bdev_out;

no_real_device:
	vfs_page_cache_mark_clean(pc);  /* I/O成功に伴ってページの状態をCLEANに遷移する */

	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

io_finished:
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュの参照を解放 */

	return 0;

put_bdev_out:
	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

unref_pc_out:
	vfs_page_cache_ref_dec(pc);  /* ページキャッシュの参照を解放 */

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
