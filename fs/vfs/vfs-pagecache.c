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

#include <fs/vfs/vfs-internal.h>

static kmem_cache page_cache_cache; /**< ページキャッシュのSLABキャッシュ */

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
	pc->pc_pf = NULL;            /* ページフレーム情報をNULLに設定 */
	pc->pc_data = NULL;          /* データページをNULLに設定 */
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

#if 0   /* TODO: ページキャッシュプールのIFに置き換え */
	/* ページキャッシュプールのmutexロックを獲得する */
	rc = mutex_lock(&pc->pc_pcplink->pcp_mtx);
	if ( rc != 0 )
		goto unref_pcp_out;


	rc = invalidate_page_cache_common(pc);  /* ページキャッシュを無効化する */
	if ( rc != 0 )
		goto unlock_pcp_out;

	/* ページキャッシュプールのロックを解放する */
	mutex_unlock(&pc->pc_pcplink->pcp_mtx);
#endif

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
   ページキャッシュ機構を初期化する
 */
void
vfs_init_pagecache(void){
	int rc;

	/* ページキャッシュのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&page_cache_cache, "Page cache cache",
	    sizeof(vfs_page_cache), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   ページキャッシュ機構を終了する
 */
void
vfs_finalize_pagecache(void){

	/* ページキャッシュのキャッシュを解放する */
	slab_kmem_cache_destroy(&page_cache_cache);
}
