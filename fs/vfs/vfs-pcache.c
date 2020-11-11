/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  page I/O cache (page cache) routines                              */
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
   ページキャッシュを解放する (内部関数)
   @param[in] pc ページキャッシュ
 */
static void
free_page_cache(vfs_page_cache *pc){

	kassert( refcnt_read(&pc->pc_refs) == 0 ); /* 参照者がいないことを確認 */
	kassert( pc->pc_pcplink != NULL );  /* ページキャッシュプールへの参照が有効 */
	list_not_linked(&pc->pc_lru_link);  /* LRUにつながっていない */

#if 0
	/* ページキャッシュへの参照をとらずにブロックバッファを解放する */
	if ( !queue_is_empty( &pc->pc_buf_que ) )
		unmap_block_buffers(pc);  /* ブロックバッファを解放 */
#endif

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
   ページキャッシュを割り当てる
   @param[in] pcp ページキャッシュを指し示すポインタのアドレス
   @retval    0       正常終了
   @retval   -ENOMEM  メモリ不足
 */
int
vfs_page_cache_alloc(vfs_page_cache **pcp){
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
	int           rc;
	bool         res;
	vfs_file_stat st;

	res = vfs_page_cache_ref_inc(pc);  /* ページキャッシュへの参照を得る */
	if ( !res ) {

		rc = -ENOENT;  /* ページキャッシュが解放中だった */
		goto error_out;
	}

	kassert( pc->pc_pcplink != NULL ); /* ページキャッシュプールにつながっている */

	/* ページキャッシュプールの参照を獲得する */
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert(res);  /* ページキャッシュが空ではないので参照獲得可能 */

	if ( !VFS_PCACHE_IS_DEVICE_PAGE(pc) ) {

		rc = -ENODEV; /* ブロックデバイス上のページキャッシュではない */
		goto unref_pcp_out;
	}

	if ( devidp != NULL ) { /* ブロックデバイスのデバイスIDを返却する */

		kassert( pc->pc_pcplink->pcp_vnode != NULL );
		res = vfs_vnode_ref_inc(pc->pc_pcplink->pcp_vnode);
		kassert(res);  /* ページキャッシュプールが存在するので参照獲得可能 */

		rc = vfs_getattr(pc->pc_pcplink->pcp_vnode, VFS_VSTAT_MASK_RDEV, &st);
		if ( rc != 0 )
			goto unref_vnode_out;

		if ( st.st_rdev == VFS_VSTAT_INVALID_DEVID ) {

			rc = -ENODEV; /* デバイスのページプールではない */
			goto unref_vnode_out;
		}

		*devidp = st.st_rdev; /* デバイスIDを返却する */

		vfs_vnode_ref_dec(pc->pc_pcplink->pcp_vnode);  /* v-nodeへの参照を解放する */
	}
	/* ページキャッシュプールの参照を減算する */
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュへの参照を返却する */

	return 0;

unref_vnode_out:
	vfs_vnode_ref_dec(pc->pc_pcplink->pcp_vnode); /* v-nodeへの参照を解放する */

unref_pcp_out:
	vfs_page_cache_pool_ref_dec(pc->pc_pcplink); /* ページキャッシュプールの参照を減算する */

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
   ブロックデバイス上のページキャッシュの読み込み/書き込みを行う
   @param[in] pc ページキャッシュ
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
   @retval -ENOENT  ページキャッシュが解放中だった
 */
int
vfs_page_cache_sync(vfs_page_cache *pc){
	int              rc;
	bool            res;
	size_t        pgsiz;
	vnode           *vn;

	res = vfs_page_cache_ref_inc(pc);  /* 操作用にページキャッシュの参照を獲得 */
	if ( !res ) {

		rc = -ENOENT;
		goto error_out;
	}

	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* 使用権があることを確認 */

	/* 二次記憶との一貫性が保たれている場合はI/Oをスキップする */
	if ( VFS_PCACHE_IS_VALID(pc) && !VFS_PCACHE_IS_DIRTY(pc) )
		goto io_finished;

	kassert( pc->pc_pcplink != NULL );
	res = vfs_page_cache_pool_ref_inc(pc->pc_pcplink);
	kassert(res);  /* ページキャッシュが空ではないので参照獲得可能 */

	rc = vfs_page_cache_pool_pagesize_get(pc->pc_pcplink, &pgsiz);
	kassert( rc == 0 );  /* ページサイズ獲得可能 */

	kassert( pc->pc_pcplink->pcp_vnode != NULL );
	res = vfs_vnode_ref_inc(pc->pc_pcplink->pcp_vnode);
	kassert(res);  /* ページキャッシュプールが存在するので参照獲得可能 */

	vn = pc->pc_pcplink->pcp_vnode; /* ページプールのv-node */

	kassert( vn->v_mount != NULL );
	kassert( vn->v_mount->m_fs != NULL );

	/*
	 * ページの読み書きを実施
	 */
	if ( VFS_PCACHE_IS_DIRTY(pc) ) {

		/*
		 * ページキャッシュを書き戻す
		 */

		/* 必要に応じて書き戻すための前処理を実行
		 */
		if ( vn->v_mount->m_fs->c_calls->fs_page_write != NULL ){

			rc = vn->v_mount->m_fs->c_calls->fs_page_prepare_write(
			vn->v_mount->m_fs_super,
			vn->v_id,
			vn->v_fs_vnode, pc->pc_offset, pc->pc_data, pgsiz);
			if ( rc != 0 )
				goto unref_vnode_out;

		}

		if ( vn->v_mount->m_fs->c_calls->fs_page_write == NULL )
			goto io_finished;

		/* ページを書き戻す
		 */
		rc = vn->v_mount->m_fs->c_calls->fs_page_write(vn->v_mount->m_fs_super,
		    vn->v_id, vn->v_fs_vnode, pc->pc_offset, pc->pc_data, pgsiz);
		if ( rc != 0 )
			goto unref_vnode_out;

	} else if ( !VFS_PCACHE_IS_VALID(pc) ) {

		if ( vn->v_mount->m_fs->c_calls->fs_page_read == NULL )
			goto io_finished;

		/* ページを読み込む
		 */
		rc = vn->v_mount->m_fs->c_calls->fs_page_read(vn->v_mount->m_fs_super,
		    vn->v_id, vn->v_fs_vnode, pc->pc_offset, pc->pc_data, pgsiz);
		if ( rc != 0 )
			goto unref_vnode_out;

	}

	/* I/O成功に伴ってページの状態をCLEANに遷移する */
	vfs_page_cache_mark_clean(pc);

io_finished:
	vfs_vnode_ref_dec(pc->pc_pcplink->pcp_vnode);

	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュの参照を解放 */

	return 0;

unref_vnode_out:
	vfs_vnode_ref_dec(pc->pc_pcplink->pcp_vnode);

	vfs_page_cache_pool_ref_dec(pc->pc_pcplink);

	vfs_page_cache_ref_dec(pc);  /* ページキャッシュの参照を解放 */

error_out:
	return rc;
}


/**
   ページキャッシュ機構を初期化する
 */
void
vfs_pagecache_init(void){
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
vfs_pagecache_finalize(void){

	/* ページキャッシュのキャッシュを解放する */
	slab_kmem_cache_destroy(&page_cache_cache);
}
