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
static bdev_db __unused g_bdevdb = __BDEVDB_INITIALIZER(&g_bdevdb);

static kmem_cache bio_req_ent_cache;  /**< BIOリクエストエントリのSLABキャッシュ    */
static kmem_cache bio_req_cache;      /**< BIOリクエストのSLABキャッシュ            */
static kmem_cache bdev_ent_cache;     /**< ブロックデバイスエントリのSLABキャッシュ */

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
   ブロックI/Oリクエストエントリを初期化する (内部関数)
   @param[in] ent ブロックI/Oリクエストエントリ
 */
static void __unused
init_bio_request_entry(bio_request_entry *ent){

	list_init(&ent->bre_ent);           /* リストエントリの初期化 */
	ent->bre_start = 0;                 /* 転送開始ページ位置の初期化 */
	ent->bre_direction = BIO_DIR_READ;  /* 読み取りに設定 */
	ent->bre_status = BIO_STATE_NONE;   /* 初期状態に設定 */
	ent->bre_error = 0;                 /* エラーなし */
	ent->bre_breqp = NULL;              /* BIOリクエストを初期化 */
	ent->bre_page =  NULL;              /* ページキャッシュを初期化 */
}

/**
   ブロックI/Oリクエストを初期化する (内部関数)
   @param[in] req ブロックI/Oリクエスト
 */
static void __unused
init_bio_request(bio_request *req){

	spinlock_init(&req->br_lock);        /*  ロックの初期化  */
	list_init(&req->br_ent);             /* リストエントリの初期化 */
	wque_init_wait_queue(&req->br_wque); /* ウエイトキューの初期化 */
	queue_init(&req->br_req);            /* リクエストキューの初期化 */
	req->br_bdevp = NULL;                /* ブロックデバイスへのポインタを初期化 */
}

/**
   ブロックデバイスエントリを割り当てる (内部関数)
   @param[in] entp ブロックデバイスエントリを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int __unused
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
   ブロックI/Oリクエストエントリを割り当てる (内部関数)
   @param[in] entp  ブロックI/Oリクエストエントリを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int __unused
alloc_bio_request_entry(bio_request_entry **entp){
	int                 rc;
	bio_request_entry *ent;

	/* BIOリクエストエントリを確保する */
	rc = slab_kmem_cache_alloc(&bio_req_ent_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bio_request_entry(ent);  /* 確保したエントリを初期化する */

	if ( entp != NULL )
		*entp = ent;  /* 確保したエントリを返却する */

	return 0;

error_out:
	return rc;
}

/**
   ブロックI/Oリクエストを割り当てる (内部関数)
   @param[in] entp  ブロックI/Oリクエストを指し示すポインタのアドレス
   @retval  0       正常終了
   @retval -ENOMEM  メモリ不足
 */
static int __unused
alloc_bio_request(bio_request **entp){
	int           rc;
	bio_request *ent;

	/* BIOリクエストを確保する */
	rc = slab_kmem_cache_alloc(&bio_req_cache, KMALLOC_NORMAL, (void **)&ent);
	if ( rc != 0 )
		goto error_out;

	init_bio_request(ent);  /* 確保したリクエストを初期化する */

	if ( entp != NULL )
		*entp = ent;  /* 確保したリクエストを返却する */

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
   ブロックI/Oリクエストエントリを解放する (内部関数)
   @param[in] ent   ブロックI/Oリクエストエントリ
 */
static void __unused
free_bio_request_entry(bio_request_entry *ent){

	kassert(list_not_linked(&ent->bre_ent));  /* リクエストにつながっていないことを確認 */

	vfs_page_cache_ref_dec(ent->bre_page); /* ページキャッシュの参照を解放 */
	slab_kmem_cache_free((void *)ent); /* BIOリクエストエントリを解放する */
}

/**
   ブロックI/Oリクエストを解放する (内部関数)
   @param[in] ent  ブロックI/Oリクエスト
 */
static void __unused
free_bio_request(bio_request *ent){

	/* ブロックデバイスのリクエストキューにつながっていないことを確認 */
	kassert(list_not_linked(&ent->br_ent));

	/* リクエストが空であることを確認 */
	kassert( queue_is_empty(&ent->br_req) );

	wque_wakeup(&ent->br_wque, WQUE_DESTROYED);  /* 待ちスレッドを起床 */

	slab_kmem_cache_free((void *)ent); /* BIOリクエストを解放する */
}

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
   ブロックデバイス機構の初期化
 */
void
bdev_init(void){
	int rc;

	/* BIOリクエストエントリのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&bio_req_ent_cache, "BIO request entry cache",
	    sizeof(bio_request_entry), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

	/* BIOリクエストのキャッシュを初期化する */
	rc = slab_kmem_cache_create(&bio_req_cache, "BIO request cache",
	    sizeof(bio_request), SLAB_ALIGN_NONE,  0, KMALLOC_NORMAL, NULL, NULL);
	kassert( rc == 0 );

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

	/* BIOリクエストエントリキャッシュを解放 */
	slab_kmem_cache_destroy(&bio_req_ent_cache);
	/* BIOリクエストキャッシュを解放 */
	slab_kmem_cache_destroy(&bio_req_cache);
	 /* ブロックデバイスエントリキャッシュを解放 */
	slab_kmem_cache_destroy(&bdev_ent_cache);
}
