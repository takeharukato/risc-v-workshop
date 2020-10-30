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

/**
   ブロックデバイスデータベース
 */
static bdev_db g_bdevdb = __BDEVDB_INITIALIZER(&g_bdevdb);

static kmem_cache bdev_ent_cache; /**< ブロックデバイスエントリのSLABキャッシュ */
static kmem_cache blkbuf_cache;   /**< ブロックバッファのSLABキャッシュ */

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
static void __unused
init_bdev_entry(bdev_entry *ent){

	spinlock_init(&ent->bdent_lock);               /*  ロックの初期化  */
	refcnt_init(&ent->bdent_refs);            /* 参照カウンタの初期化  */
	ent->bdent_devid = VFS_VSTAT_INVALID_DEVID;  /* デバイスIDの初期化 */
	ent->bdent_blksiz = 0;                   /* ブロックサイズの初期化 */
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
static void __unused
free_bdev_entry(bdev_entry *ent){

	kassert( refcnt_read(&ent->bdent_refs) == 0 ); /* 参照者がいないことを確認 */
	kassert( queue_is_empty(&ent->bdent_rque) );  /* キューが空であることを確認 */
	slab_kmem_cache_free((void *)ent); /* ブロックデバイスエントリを解放する */

	return ;
}

/**
   ブロックバッファを割り当てる (内部関数)
   @param[in] bufp  ブロックバッファを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int
alloc_blkbuf(block_buffer **bufp){
	int            rc;
	block_buffer *buf;

	kassert( bufp != NULL );

	/* ブロックバッファを確保する */
	rc = slab_kmem_cache_alloc(&blkbuf_cache, KMALLOC_NORMAL, (void **)&buf);
	if ( rc != 0 )
		goto error_out;

	list_init(&buf->b_ent); /* リストエントリの初期化 */
	buf->b_offset = 0;  /* バッファオフセットを初期化する */
	buf->b_len = 0;     /* バッファ長を初期化する */
	buf->b_page = NULL; /* ページキャッシュポインタを初期化する */

	*bufp = buf;  /* 確保したバッファを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックバッファを解放する (内部関数)
   @param[in] buf   ブロックバッファ
 */
static void
free_blkbuf(block_buffer *buf){

	slab_kmem_cache_free((void *)buf);  /* ブロックバッファを解放する */
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

/**
   ブロックデバイスエントリの参照を獲得する (内部関数)
   @param[in] devid  ブロックデバイスのデバイスID
   @param[out] bdevp ブロックデバイスエントリを指し示すポインタのアドレス
   @retval  0      正常終了
   @retval -ENODEV ブロックデバイスエントリが見つからなかった
 */
static int
get_bdev_entry(dev_id devid, bdev_entry **bdevp){
	int                 rc;
	bool               res;
	bdev_entry       *bdev;
	bdev_entry         key;
	intrflags       iflags;

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

	res = inc_bdev_ent_ref(bdev);  /* 参照を加算 */
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
   ブロックデバイスエントリの参照を解放する (内部関数)
   @param[in] devid  ブロックデバイスのデバイスID
   @param[out] bdevp ブロックデバイスエントリを指し示すポインタのアドレス
   @retval  0      ブロックデバイスの最終参照者だった
   @retval -EBUSY  ブロックデバイスの最終参照者ではなかった
 */
static int
put_bdev_entry(bdev_entry *bdev){
	bool res;

	res = dec_bdev_ent_ref(bdev); /* ブロックデバイスエントリへの参照を解放する */
	if ( !res )
		return -EBUSY; /* ブロックデバイスの最終参照者ではなかった */

	return 0; /* ブロックデバイスの最終参照者だった */
}

/**
   ページキャッシュに割り当てられたブロックバッファを解放する (内部関数)
   @param[in]  pcache   ページキャッシュ
 */
static void
unmap_page_cache_blocks(vfs_page_cache *pc){
	block_buffer *cur_buf;
	list        *cur, *np;

	/* ブロックバッファを解放する
	 */
	queue_for_each_safe(cur, &pc->pc_buf_que, np) {

		/* 先頭のバッファを取り出す */
		cur_buf = container_of(queue_get_top(&pc->pc_buf_que), block_buffer, b_ent);
		free_blkbuf(cur_buf);  /* バッファを解放する */
	}

	return ;
}

/**
   ページキャッシュにブロックバッファを割り当てる (内部関数)
   @param[in]  bdev     ブロックデバイスエントリ
   @param[in]  pcache   ページキャッシュ
   @retval  0 正常終了
 */
static int
map_page_cache_blocks(bdev_entry *bdev, vfs_page_cache *pc){
	int                rc;
	bool              res;
	block_buffer     *buf;
	off_t         blk_off;
	obj_cnt_type        i;
	obj_cnt_type  nr_bufs;

	res = inc_bdev_ent_ref(bdev);  /* 参照を加算 */
	kassert( res );  /* ブロックデバイスDBからの参照が残っている */

	kassert( pc->pc_pcplink != NULL );
	kassert( pc->pc_pcplink->pcp_pgsiz >= bdev->bdent_blksiz );
	kassert( !addr_not_aligned(pc->pc_pcplink->pcp_pgsiz, bdev->bdent_blksiz) );

	/* ページ内に割り当てるブロック数を算出する */
	nr_bufs = pc->pc_pcplink->pcp_pgsiz / bdev->bdent_blksiz;

	/* ページキャッシュにブロックを割り当てる
	 */
	for( i = 0, blk_off = 0; nr_bufs > i; ++i ) {

		rc = alloc_blkbuf(&buf);
		if ( rc != 0 )
			goto free_blocks_out;

		buf->b_offset = blk_off;  /* ページ内オフセットを設定 */
		buf->b_len = bdev->bdent_blksiz; /* ブロックサイズを設定 */
		buf->b_page = pc; /* ページキャッシュを設定 */
		queue_add(&pc->pc_buf_que, &buf->b_ent); /* ページキャッシュに追加 */

		blk_off += bdev->bdent_blksiz; /* ページ内オフセットを更新 */
	}

	res = dec_bdev_ent_ref(bdev); /* ブロックデバイスエントリへの参照を解放する */
	kassert( !res );

	return 0;

free_blocks_out:

	unmap_page_cache_blocks(pc); /* ブロックバッファを解放する */

	res = dec_bdev_ent_ref(bdev); /* ブロックデバイスエントリへの参照を解放する */
	kassert( !res );

	return rc;
}

/*
 * IF関数
 */

/**
   ブロックデバイスエントリにページキャッシュプールを設定する
   @param[in] bdev ブロックデバイスエントリ
   @param[in] pool ページキャッシュプール
   @retval  0      正常終了
   @retval -EBUSY  ページキャッシュプールが既に割り当てられている
 */
int
bdev_page_cache_pool_set(bdev_entry *bdev, vfs_page_cache_pool *pool){
	int                    rc;
	intrflags          iflags;

	/* ブロックデバイスエントリをロック */
	spinlock_lock_disable_intr(&bdev->bdent_lock, &iflags);

	if ( bdev->bdent_pool != NULL ) {

		rc = -EBUSY;  /* ページキャッシュプール設定済み */
		goto error_out;
	}

	bdev->bdent_pool = pool;  /* ページキャッシュプールを設定する  */

	/* ブロックデバイスエントリをアンロック */
	spinlock_unlock_restore_intr(&bdev->bdent_lock, &iflags);

	return 0;

error_out:
	return rc;
}

/**
   ブロックデバイス上のページキャッシュを獲得する
   @param[in]  devid    デバイスID
   @param[in]  offset   デバイス先頭からのオフセットアドレス(単位:バイト)
   @param[out] pcachep  ページキャッシュを指し示すポインタのアドレス
   @retval  0 正常終了
   @retval -ENOENT デバイスIDが不正
 */
int
bdev_page_cache_get(dev_id devid, off_t offset, vfs_page_cache **pcachep){
	int             rc;
	bdev_entry   *bdev;
	vfs_page_cache *pc;

	rc = get_bdev_entry(devid, &bdev);   /* ブロックデバイスエントリへの参照を獲得 */
	if ( rc != 0 )
		goto error_out;

	kassert( bdev->bdent_pool != NULL ); /* ページキャッシュプール設定済み */
	kassert( bdev->bdent_calls.fs_strategy != NULL ); /* I/Oハンドラが提供されている */

	/* ページキャッシュを獲得する */
	rc = vfs_page_cache_get(bdev->bdent_pool, offset, &pc);
	if ( rc != 0 )
		goto put_bdev_out;

	if ( queue_is_empty(&pc->pc_buf_que) )
		map_page_cache_blocks(bdev, pc);  /* ページキャッシュにブロックを割り当てる */

	if ( !VFS_PCACHE_IS_VALID(pc) ) {  /* 未読み込みのページの場合 */

		rc = bdev->bdent_calls.fs_strategy(pc); /* ページの内容を読み込む */
		if ( rc != 0 )
			goto unmap_blocks_out;
	}

	vfs_page_cache_put(pc);  /* ページキャッシュの参照を解放する */

	put_bdev_entry(bdev);  /* ブロックデバイスエントリへの参照を解放 */

	return 0;

unmap_blocks_out:
	if ( !queue_is_empty(&pc->pc_buf_que) )
		unmap_page_cache_blocks(pc); /* ページキャッシュのブロックを解放 */

	vfs_page_cache_put(pc);  /* ページキャッシュの参照を解放する */

put_bdev_out:
	put_bdev_entry(bdev);  /* ブロックデバイスエントリへの参照を解放 */

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
		return -EINVAL;

	rc = alloc_bdev_entry(&ent);
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

	spinlock_lock_disable_intr(&g_bdevdb.bdev_lock, &iflags);

	/* デバイスドライバを登録 */
	res = RB_INSERT(_bdev_tree, &g_bdevdb.bdev_head,  ent);

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
	dec_bdev_ent_ref(ent);

error_out:
	return rc;
}

/**
   ブロックデバイスドライバの登録を抹消する
   @param[in] devid   デバイスID
 */
void
bdev_device_unregister(dev_id devid){
	bdev_entry   key;
	bdev_entry  *ent;
	bdev_entry  *res;
	intrflags iflags;

	if ( devid == VFS_VSTAT_INVALID_DEVID )
		return ;

	spinlock_lock_disable_intr(&g_bdevdb.bdev_lock, &iflags);

	key.bdent_devid = devid;
	ent = RB_FIND(_bdev_tree, &g_bdevdb.bdev_head, &key);
	if ( ent == NULL ) {

		spinlock_unlock_restore_intr(&g_bdevdb.bdev_lock, &iflags);
		return;  /* 対象のデバイスが削除済み */
	}

	res = RB_REMOVE(_bdev_tree, &g_bdevdb.bdev_head, ent);
	kassert( res != NULL );

	spinlock_unlock_restore_intr(&g_bdevdb.bdev_lock, &iflags);

	/* ページキャッシュプールへの参照を解放 */
	vfs_page_cache_pool_ref_dec(ent->bdent_pool);
	ent->bdent_pool = NULL;

	dec_bdev_ent_ref(ent);

	return ;
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

	/* ブロックバッファのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&blkbuf_cache, "block buffer cache",
	    sizeof(block_buffer), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );
}

/**
   ブロックデバイス機構の終了
 */
void
bdev_finalize(void){

	 /* ブロックデバイスエントリキャッシュを解放 */
	slab_kmem_cache_destroy(&bdev_ent_cache);
	 /* ブロックバッファを解放 */
	slab_kmem_cache_destroy(&blkbuf_cache);
}
