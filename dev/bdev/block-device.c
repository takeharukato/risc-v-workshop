/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  Block device Routines                                             */
/*                                                                    */
/**********************************************************************/

#include <klib/freestanding.h>
#include <kern/kern-common.h>

#include <kern/spinlock.h>
#include <klib/refcount.h>

#include <kern/page-if.h>

#include <kern/dev-if.h>
#include <kern/vfs-if.h>

#include <fs/vfs/vfs-internal.h>

/**
   ブロックデバイスデータベース
 */
static bdev_db g_bdevdb = __BDEVDB_INITIALIZER(&g_bdevdb);

static kmem_cache bdev_ent_cache; /**< ブロックデバイスエントリのSLABキャッシュ */

/**
    ブロックデバイスデータベース赤黒木
 */
static int _bdev_ent_cmp(struct _bdev_entry *_key, struct _bdev_entry *_ent);
RB_GENERATE_STATIC(_bdev_tree, _bdev_entry, bdent_ent, _bdev_ent_cmp);

/**
    ブロックデバイスデータベースエントリ比較関数
    @param[in] key 比較対象1
    @param[in] ent RB木内の各エントリ
    @retval 正  keyのデバイスIDがentより大きい
    @retval 負  keyのデバイスIDがentより小さい
    @retval 0   keyのデバイスIDがentに等しい
 */
static int
_bdev_ent_cmp(struct _bdev_entry *key, struct _bdev_entry *ent){

	if ( key->bdent_devid > ent->bdent_devid )
		return 1;

	if ( key->bdent_devid < ent->bdent_devid )
		return -1;

	return 0;
}

/**
   ブロックデバイスエントリを初期化する (内部関数)
   @param[in] ent ブロックデバイスエントリ
 */
static void
init_bdev_entry(bdev_entry *ent){

	spinlock_init(&ent->bdent_lock);               /*  ロックの初期化  */
	refcnt_init(&ent->bdent_refs);            /* 参照カウンタの初期化  */
	ent->bdent_devid = VFS_VSTAT_INVALID_DEVID;  /* デバイスIDの初期化 */
	ent->bdent_secsiz = BDEV_DEFAULT_SECSIZ;     /* セクタ長の初期化   */
	ent->bdent_blksiz = BDEV_DEFAULT_BLKSIZ; /* ブロックサイズの初期化 */
	ent->bdent_capacity = BDEV_DEFAULT_CAPACITY; /* 容量の初期化       */
	queue_init(&ent->bdent_rque);          /* リクエストキューの初期化 */
	vfs_fs_calls_init(&ent->bdent_calls);   /*  ファイル操作IFの初期化 */
	ent->bdent_private = NULL;             /* プライベート情報の初期化 */
}

/**
   ブロックデバイスエントリを割り当てる (内部関数)
   @param[in] entp ブロックデバイスエントリを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int
alloc_bdev_entry(bdev_entry **entp){
	int          rc;
	bdev_entry *ent;

	/* ブロックデバイスエントリを確保する */
	rc = slab_kmem_cache_alloc(&bdev_ent_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bdev_entry(ent);  /* 確保したエントリを初期化する */

	if ( entp != NULL )
		*entp = ent;  /* 確保したエントリを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックデバイスエントリを解放する (内部関数)
   @param[in] ent   ブロックデバイスエントリ
 */
static void
free_bdev_entry(bdev_entry *ent){

	kassert( refcnt_read(&ent->bdent_refs) == 0 ); /* 参照者がいないことを確認 */
	kassert( queue_is_empty(&ent->bdent_rque) );  /* キューが空であることを確認 */
	slab_kmem_cache_free((void *)ent); /* ブロックデバイスエントリを解放する */

	return ;
}

/**
   ブロックデバイスエントリへの参照を加算する (内部関数)
   @param[in] ent ブロックデバイスエントリ
   @retval    真  ブロックデバイスエントリの参照を獲得できた
   @retval    偽  ブロックデバイスエントリの参照を獲得できなかった
 */
static bool
inc_bdev_ent_ref(bdev_entry *ent){

	/* ブロックデバイスエントリ解放中でなければ, 参照カウンタを加算
	 */
	return ( refcnt_inc_if_valid(&ent->bdent_refs) != 0 );
}

/**
   ブロックデバイスエントリへの参照を減算する (内部関数)
   @param[in] ent ブロックデバイスエントリ
   @retval    真 ブロックデバイスエントリの最終参照者だった
   @retval    偽 ブロックデバイスエントリの最終参照者でない
 */
static bool
dec_bdev_ent_ref(bdev_entry *ent){
	bool res;

	/* ブロックデバイスエントリ解放中でなければ, 参照カウンタを減算
	 */
	res = refcnt_dec_and_test(&ent->bdent_refs);
	if ( res )  /* 最終参照者だった場合 */
		free_bdev_entry(ent);

	return res;
}

/*
 * IF関数
 */
/**
   ブロックデバイスエントリの参照を獲得する
   @param[in] devid  ブロックデバイスのデバイスID
   @param[out] bdevp ブロックデバイスエントリを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV ブロックデバイスエントリが見つからなかった
 */
int
bdev_bdev_entry_get(dev_id devid, bdev_entry **bdevp){
	int                 rc;
	bool               res;
	bdev_entry       *bdev;
	bdev_entry         key;
	intrflags       iflags;

	if ( devid == VFS_VSTAT_INVALID_DEVID )
		return -EINVAL;  /* デバイスIDが不正 */

	key.bdent_devid = devid;  /* デバイスIDを設定 */

	/* ブロックデバイスDBのロックを獲得 */
	spinlock_lock_disable_intr(&g_bdevdb.bdev_lock, &iflags);

	/*
	 * ブロックデバイスを検索する
	 */
	bdev = RB_FIND(_bdev_tree, &g_bdevdb.bdev_head, &key);
	if ( bdev == NULL ) {

		rc = -ENODEV;  /* デバイスが見つからなかった */
		goto unlock_out;
	}

	res = inc_bdev_ent_ref(bdev);  /* スレッドからの参照を加算 */
	kassert( res );  /* ブロックデバイスDBからの参照が残っている */

	/* ブロックデバイスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_bdevdb.bdev_lock, &iflags);

	if ( bdevp != NULL )
		*bdevp = bdev;  /* ブロックデバイスエントリを返却 */

	return 0;

unlock_out:
	/* ブロックデバイスDBのロックを解放 */
	spinlock_unlock_restore_intr(&g_bdevdb.bdev_lock, &iflags);
	return rc;
}

/**
   ブロックデバイスエントリの参照を解放する
   @param[in] bdev ブロックデバイスエントリ
   @retval  0      ブロックデバイスの最終参照者だった
   @retval -EBUSY  ブロックデバイスの最終参照者ではなかった
 */
int
bdev_bdev_entry_put(bdev_entry *bdev){
	bool res;

	res = dec_bdev_ent_ref(bdev); /* ブロックデバイスエントリへの参照を解放する */
	if ( !res )
		return -EBUSY; /* ブロックデバイスの最終参照者ではなかった */

	return 0; /* ブロックデバイスの最終参照者だった */
}

/**
   ブロックデバイスエントリにページキャッシュプールを設定する
   @param[in] bdev ブロックデバイスエントリ
   @param[in] pool ページキャッシュプール
   @retval  0      正常終了
   @retval -EBUSY  ブロックデバイスエントリにページキャッシュプールが既に割り当てられているか
   ページキャッシュプールにブロックデバイスが割り当てられている
   @retval -EINVAL ブロックデバイスのデバイスIDが不正
 */
int
bdev_page_cache_pool_set(bdev_entry *bdev, vfs_page_cache_pool *pool){
	int                    rc;
	intrflags          iflags;

	/* ブロックデバイスエントリをロック */
	spinlock_lock_disable_intr(&bdev->bdent_lock, &iflags);

	if ( bdev->bdent_pool != NULL ) {

		rc = -EBUSY;  /* ページキャッシュプール設定済み */
		goto unlock_out;
	}

	if ( pool->pcp_bdevid != VFS_VSTAT_INVALID_DEVID ) {

		rc = -EBUSY;  /* ページキャッシュプール設定済み */
		goto unlock_out;
	}

	if ( bdev->bdent_devid == VFS_VSTAT_INVALID_DEVID ){

		rc = -EINVAL;  /* 不正ブロックデバイスID */
		goto unlock_out;
	}

	bdev->bdent_pool = pool;  /* ページキャッシュプールを設定する  */
	pool->pcp_bdevid = bdev->bdent_devid; /* ブロックデバイスのデバイスIDを設定 */

	/* ブロックデバイスエントリをアンロック */
	spinlock_unlock_restore_intr(&bdev->bdent_lock, &iflags);

	return 0;

unlock_out:
	/* ブロックデバイスエントリをアンロック */
	spinlock_unlock_restore_intr(&bdev->bdent_lock, &iflags);
	return rc;
}

/**
   ブロックデバイスのブロックサイズを設定する
   @param[in]  devid    デバイスID
   @param[in]  blksiz   デバイスのブロックサイズ(単位:バイト)
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正, または, ブロックサイズが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_blocksize_set(dev_id devid, size_t blksiz){
	int             rc;
	bdev_entry   *bdev;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		return rc;

	if ( !BDEV_IS_BLOCKSIZE_VALID(bdev->bdent_secsiz, blksiz) ) {

		rc = -EINVAL;  /* ブロックサイズが不正 */
		goto put_bdev_out;
	}

	bdev->bdent_blksiz = blksiz;  /* ブロックサイズを設定する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;

put_bdev_out:
	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return rc;
}

/**
   ブロックデバイスのブロックサイズを取得する
   @param[in]   devid     デバイスID
   @param[out]  blksizp   デバイスのブロックサイズを返却する領域
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_blocksize_get(dev_id devid, size_t *blksizp){
	int             rc;
	bdev_entry   *bdev;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		return rc;

	if ( blksizp != NULL )
		*blksizp = bdev->bdent_blksiz;  /* ブロックサイズを返却する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;
}

/**
   ブロックデバイスのセクタサイズを設定する
   @param[in]  devid    デバイスID
   @param[in]  secsiz   デバイスのセクタサイズ(単位:バイト)
   @retval  0 正常終了
   @retval -EINVAL デバイスID/セクタサイズが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_sectorsize_set(dev_id devid, size_t secsiz){
	int             rc;
	bdev_entry   *bdev;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		return rc;

	if ( !BDEV_IS_SECTORSIZE_VALID(secsiz) )  {

		rc = -EINVAL;  /* セクタサイズが不正 */
		goto put_bdev_out;
	}

	/* ブロックサイズよりセクタサイズの方が大きい場合, ブロックサイズを更新 */
	if ( secsiz > bdev->bdent_blksiz )
		bdev->bdent_blksiz = secsiz; /* セクタサイズをブロックサイズに設定 */

	bdev->bdent_secsiz = secsiz;  /* セクタサイズを設定する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;

put_bdev_out:
	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return rc;
}

/**
   ブロックデバイスのセクタサイズを取得する
   @param[in]   devid     デバイスID
   @param[out]  secsizp   デバイスのセクタサイズを返却する領域
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_sectorsize_get(dev_id devid, size_t *secsizp){
	int             rc;
	bdev_entry   *bdev;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		return rc;

	if ( secsizp != NULL )
		*secsizp = bdev->bdent_secsiz;  /* セクタサイズを返却する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;
}

/**
   ブロックデバイスの容量を設定する
   @param[in]  devid    デバイスID
   @param[in]  capacity デバイスの容量(単位:バイト)
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_capacity_set(dev_id devid, size_t capacity){
	int             rc;
	bdev_entry   *bdev;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		return rc;

	bdev->bdent_capacity = capacity;  /* デバイスの容量を設定する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;
}

/**
   ブロックデバイスの容量を取得する
   @param[in]   devid    デバイスID
   @param[out]  capacityp デバイスの容量を返却する領域
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_capacity_get(dev_id devid, size_t *capacityp){
	int             rc;
	bdev_entry   *bdev;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		return rc;

	if ( capacityp != NULL )
		*capacityp = bdev->bdent_capacity;  /* 容量を返却する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;
}

/**
   ブロックデバイス上のページキャッシュを獲得する
   @param[in]  devid    デバイスID
   @param[in]  offset   デバイス先頭からのオフセットアドレス(単位:バイト)
   @param[out] pcachep  ページキャッシュを指し示すポインタのアドレス
   @retval  0 正常終了
   @retval -EINVAL  デバイスIDが不正
   @retval -ENODEV  ミューテックスが破棄された, または, 指定されたデバイスが見つからなかった
   @retval -EINTR   非同期イベントを受信した
   @retval -ENOMEM  メモリ不足
   @retval -ENOENT  ページキャッシュが解放中だった
   @retval -EIO     I/Oエラー
 */
int
bdev_page_cache_get(dev_id devid, off_t offset, vfs_page_cache **pcachep){
	int             rc;
	bdev_entry   *bdev;
	vfs_page_cache *pc;

	rc = bdev_bdev_entry_get(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		goto error_out;

	kassert( bdev->bdent_pool != NULL ); /* ページキャッシュプール設定済み */
	kassert( bdev->bdent_calls.fs_strategy != NULL ); /* I/Oハンドラが提供されている */

	/* ページキャッシュを獲得する */
	rc = vfs_page_cache_get(bdev->bdent_pool, offset, &pc);
	if ( rc != 0 )
		goto put_bdev_out;

	kassert( VFS_PCACHE_IS_BUSY(pc) );  /* バッファ使用権を獲得済み */

	/* ページキャッシュにブロックを割り当てる
	 */
	if ( vfs_page_cache_is_block_buffer_empty(pc) ) { /* ブロックバッファ未割当の場合 */

		rc = block_buffer_map_to_page_cache(devid, pc);
		if ( rc != 0 ) {

			/* ブロックバッファが空になっているはず */
			kassert( vfs_page_cache_is_block_buffer_empty(pc) );
			goto put_pc_out;
		}
	}

	if ( !VFS_PCACHE_IS_VALID(pc) ) {  /* 未読み込みのページの場合 */

		rc = bio_page_read(pc);  /* ブロックデバイスから読み込む */
		if ( rc != 0 )
			goto unmap_blocks_out;
	}

	vfs_page_cache_put(pc);  /* ページキャッシュの参照を解放する */

	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;

unmap_blocks_out:
	/* ページキャッシュ中のブロックを解放 */
	if ( !vfs_page_cache_is_block_buffer_empty(pc) )
		block_buffer_unmap_from_page_cache(pc);
put_pc_out:
	vfs_page_cache_put(pc);  /* ページキャッシュの参照を解放する */

put_bdev_out:
	bdev_bdev_entry_put(bdev);  /* ブロックデバイスエントリへの参照を解放 */

error_out:
	return rc;
}

/**
   ブロックデバイスにリクエストを追加する
   @param[in] devid デバイスID
   @param[in] req   リクエスト
   @retval  0      正常終了
   @retval -EINVAL  デバイスIDが不正
   @retval -ENODEV 指定されたデバイスが見つからなかった
 */
int
bdev_add_request(dev_id devid, bio_request *req){
	int                 rc;
	bdev_entry       *bdev;
	intrflags       iflags;

	kassert( list_not_linked(&req->br_ent) ); /* 未接続のリクエスト */

	/* ブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(req->br_bdevid, &bdev);
	if ( rc != 0 )
		goto error_out;

	/* リクエストキューのロックを獲得 */
	spinlock_lock_disable_intr(&bdev->bdent_lock, &iflags);

	queue_add(&bdev->bdent_rque, &req->br_ent); /* キューに追加 */

	/* リクエストキューのロックを解放 */
	spinlock_unlock_restore_intr(&bdev->bdent_lock, &iflags);

	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックデバイスに登録されているリクエストを処理する
   @param[in] devid デバイスID
   @retval  0      正常終了
   @retval -ENODEV 指定されたデバイスが見つからなかった, または,
                   リクエスト中のページキャッシュがブロックデバイスのページキャッシュではない
   @retval -EINVAL  デバイスIDが不正
   @retval -ENOENT  ページキャッシュが解放中だった
 */
int
bdev_handle_requests(dev_id devid){
	int                 rc;
	bdev_entry       *bdev;
	bio_request       *req;
	intrflags       iflags;

	/* ブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(devid, &bdev);
	if ( rc != 0 )
		goto error_out;

	/* リクエストキューのロックを獲得 */
	spinlock_lock_disable_intr(&bdev->bdent_lock, &iflags);
	while( !queue_is_empty(&bdev->bdent_rque) ) {

		req = container_of(queue_get_top(&bdev->bdent_rque), bio_request, br_ent);

		/* リクエストキューのロックを解放 */
		spinlock_unlock_restore_intr(&bdev->bdent_lock, &iflags);

		/* @note 同期リクエストの場合は, リクエストを要求スレッドがリクエストを解放し,
		 * 非同期リクエストの場合は, bio_request_handle_one内でリクエストを解放する
		 * ため, 本関数では, リクエストを解放してはいけない。
		 */
		 bio_request_handle_one(req); /* リクエストを実行 */

		/* リクエストキューのロックを獲得 */
		spinlock_lock_disable_intr(&bdev->bdent_lock, &iflags);
	}
	/* リクエストキューのロックを解放 */
	spinlock_unlock_restore_intr(&bdev->bdent_lock, &iflags);

	bdev_bdev_entry_put(bdev); /* ブロックデバイスエントリへの参照を解放する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックデバイスドライバを登録する
   @param[in] devid   デバイスID
   @param[in] blksiz  ブロックサイズ(単位:バイト)
   @param[in] ops     ファイルシステムコール
   @param[in] private ドライバ固有情報のアドレス
   @retval  0 正常終了
   @retval -EINVAL デバイスIDが不正
   @retval -EBUSY  同一デバイスIDのドライバが登録されている
 */
int
bdev_device_register(dev_id devid, size_t blksiz, fs_calls *ops, bdev_private private){
	int           rc;
	bdev_entry  *ent;
	bdev_entry  *res;
	intrflags iflags;

	kassert( ops != NULL );

	if ( devid == VFS_VSTAT_INVALID_DEVID )
		return -EINVAL;  /* 不正デバイスID */

	rc = alloc_bdev_entry(&ent);  /* ブロックデバイスエントリを割り当てる */
	if ( rc != 0 )
		goto error_out;

	rc = vfs_dev_page_cache_pool_alloc(ent); /* ページキャッシュプールを設定する */
	if ( rc != 0 )
		goto put_bdev_ent_out;  /* 参照を減算してブロックデバイスエントリを解放 */

	kassert( ent->bdent_pool != NULL );

	ent->bdent_devid = devid;      /* デバイスIDを設定 */
	ent->bdent_blksiz = blksiz;    /* ブロックサイズを設定 */
	vfs_fs_calls_copy(&ent->bdent_calls, ops); /* fs_callをコピーする */
	ent->bdent_private = private;  /* ドライバ固有情報へのポインタを設定 */

	/* ブロックデバイスDBロックを獲得する */
	spinlock_lock_disable_intr(&g_bdevdb.bdev_lock, &iflags);

	/* デバイスドライバを登録 */
	res = RB_INSERT(_bdev_tree, &g_bdevdb.bdev_head,  ent);

	/* ブロックデバイスDBロックを解放する */
	spinlock_unlock_restore_intr(&g_bdevdb.bdev_lock, &iflags);

	if ( res != NULL ) {

		/* ページキャッシュプールへの参照を解放 */
		vfs_page_cache_pool_ref_dec(ent->bdent_pool);
		ent->bdent_pool = NULL;
		rc = -EBUSY;
		goto put_bdev_ent_out;
	}

	return 0;

put_bdev_ent_out:
	dec_bdev_ent_ref(ent);  /* ブロックデバイスエントリへの参照を解放 */

error_out:
	return rc;
}

/**
   ブロックデバイスドライバの登録を抹消する
   @param[in] devid   デバイスID
 */
void
bdev_device_unregister(dev_id devid){
	int           rc;
	bdev_entry *bdev;
	bdev_entry  *res;
	intrflags iflags;

	if ( devid == VFS_VSTAT_INVALID_DEVID )
		return ;

	/* 操作用にブロックデバイスエントリへの参照を獲得 */
	rc = bdev_bdev_entry_get(devid, &bdev);
	if ( rc != 0 )
		return;

	/* ブロックデバイスDBのロックを獲得する */
	spinlock_lock_disable_intr(&g_bdevdb.bdev_lock, &iflags);

	/* ブロックデバイスエントリをブロックデバイスDBから除去する */
	res = RB_REMOVE(_bdev_tree, &g_bdevdb.bdev_head, bdev);
	kassert( res != NULL );

	/* ブロックデバイスDBのロックを解放する */
	spinlock_unlock_restore_intr(&g_bdevdb.bdev_lock, &iflags);

	/* ページキャッシュプールへ参照を解放し, ページキャッシュから
	 * ページキャッシュプールへリンクを無効化
	 */
	vfs_page_cache_pool_ref_dec(bdev->bdent_pool);
	bdev->bdent_pool = NULL;

	/* ページキャッシュプールからブロックデバイスエントリへの参照を解放 */
	dec_bdev_ent_ref(bdev);

	dec_bdev_ent_ref(bdev);  /* 操作用のブロックデバイスエントリへの参照を解放 */

	return;
}

/**
   ブロックデバイス機構の初期化
 */
void
bdev_init(void){
	int rc;

	/* ブロックデバイスエントリのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&bdev_ent_cache, "bdev entry cache",
	    sizeof(bdev_entry), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   ブロックデバイス機構の終了
 */
void
bdev_finalize(void){

	 /* ブロックデバイスエントリキャッシュを解放 */
	slab_kmem_cache_destroy(&bdev_ent_cache);
}
